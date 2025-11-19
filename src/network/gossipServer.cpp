#include "network/gossipServer.hpp"

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>

#include "core/block.hpp"
#include "core/transaction.hpp"
#include "core/blockJson.hpp"

#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndPayload.hpp"
#include "dnd/dndTxValidator.hpp"

using json = nlohmann::json;

GossipServer::GossipServer(int port,
                           Blockchain& chain,
                           Mempool& mempool,
                           PeerManager* peers,
                           dnd::DndTxValidator* validator)
    : port_(port),
      chain_(chain),
      mempool_(mempool),
      peers_(peers),
      validator_(validator)
{}

// Kleine Helferfunktion: Antwort als JSON+Status setzen
static void setJsonError(httplib::Response& res,
                         const std::string& msg,
                         int status = 400)
{
    json j;
    j["ok"]    = false;
    j["error"] = msg;
    res.status = status;
    res.set_content(j.dump(2), "application/json");
}

void GossipServer::start()
{
    httplib::Server server;

    // ------------------------------------------------------
    // Health/Ping
    // ------------------------------------------------------
    server.Get("/ping", [](const httplib::Request&, httplib::Response& res) {
        json j;
        j["pong"] = true;
        res.set_content(j.dump(2), "application/json");
    });

    // ------------------------------------------------------
    // /gossip/tx
    //
    // Nimmt entweder:
    //  1) komplette Transaction (payload, senderPubKey, signature)
    //  2) reines DnD-Event-JSON (encounterId, actorId, targetId, ...)
    //
    // und packt es als Transaction in den *validierenden* Mempool.
    // ------------------------------------------------------
    server.Post("/gossip/tx", [this](const httplib::Request& req,
                                     httplib::Response& res) {
        try {
            if (req.body.empty()) {
                return setJsonError(res, "empty body");
            }

            json in;
            try {
                in = json::parse(req.body);
            } catch (...) {
                return setJsonError(res, "invalid JSON");
            }

            Transaction tx;

            const bool looksLikeFullTx =
                in.contains("payload") &&
                in.contains("senderPubKey") &&
                in.contains("signature");

            if (looksLikeFullTx) {
                // ----------------------------
                // Fall 1: komplette Transaction
                // ----------------------------
                tx.payload      = in.at("payload").get<std::vector<uint8_t>>();
                tx.senderPubkey = in.at("senderPubKey").get<std::vector<uint8_t>>();
                tx.signature    = in.at("signature").get<std::vector<uint8_t>>();
            } else {
                // ----------------------------
                // Fall 2: rohes DnD-Event JSON
                // ----------------------------
                dnd::DndEventTx evt;
                evt.encounterId = in.value("encounterId", "");

                evt.actorId     = in.value("actorId", "");
                evt.targetId    = in.value("targetId", "");

                evt.actorType   = in.value("actorType", 0);
                evt.targetType  = in.value("targetType", 0);

                evt.roll        = in.value("roll", 0);
                evt.damage      = in.value("damage", 0);
                evt.hit         = in.value("hit", false);
                evt.note        = in.value("note", "");

                evt.timestamp   = in.value("timestamp", 0ull);

                if (in.contains("senderPubKey")) {
                    evt.senderPubKey = in["senderPubKey"].get<std::vector<uint8_t>>();
                }
                if (in.contains("signature")) {
                    evt.signature = in["signature"].get<std::vector<uint8_t>>();
                }

                // Hier könntest du noch signieren, wenn du einen PrivateKey mitlieferst.
                // z.B.: dnd::signDndEvent(evt, privKey);

                // In Blockchain-Transaction einsacken
                tx.payload      = dnd::encodeDndTx(evt);
                tx.senderPubkey = evt.senderPubKey;
                tx.signature    = evt.signature;
            }

            // --------------------------------------------------
            // Optional: DnD-spezifische Validierung
            // (nutzt DndTxValidator::validate(DndEventTx&))
            // --------------------------------------------------
            if (validator_ && dnd::isDndPayload(tx.payload)) {
                auto evt = dnd::decodeDndTx(tx.payload);
                evt.senderPubKey = tx.senderPubkey;
                evt.signature    = tx.signature;

                std::string vErr;
                if (!validator_->validate(evt, vErr)) {
                    return setJsonError(res, "DnD validation failed: " + vErr, 400);
                }
            }

            // --------------------------------------------------
            // Mempool-Validation + Einfügen
            // (nutzt Blockchain::validateTransaction + DnD-Regeln)
            // --------------------------------------------------
            std::string err;
            if (!mempool_.addTransactionValidated(tx, err)) {
                return setJsonError(res, "mempool rejected tx: " + err, 400);
            }

            // --------------------------------------------------
            // Optional: an Peers broadcasten
            // --------------------------------------------------
            if (peers_) {
                peers_->broadcastTransaction(tx);
            }

            json out;
            out["ok"] = true;
            res.set_content(out.dump(2), "application/json");

        } catch (const std::exception& ex) {
            return setJsonError(res, std::string("exception: ") + ex.what(), 500);
        } catch (...) {
            return setJsonError(res, "unknown error", 500);
        }
    });

    // ------------------------------------------------------
    // /gossip/block
    //
    // Erwartet: body = Block::serialize() (rohe Bytes)
    // Schritte:
    //  - Body → std::vector<uint8_t>
    //  - Block::deserialize
    //  - Header-Signatur prüfen
    //  - prevHash == lastBlock.hash()
    //  - Höhe == last.height + 1 (oder 0, wenn Kette leer)
    //  - chain_.appendBlock(block)
    //  - Block an Peers broadcasten
    //
    // DnDState wird NICHT hier, sondern in Blockchain::appendBlock(...) aktualisiert.
    // ------------------------------------------------------
    server.Post("/gossip/block", [this](const httplib::Request& req,
                                        httplib::Response& res) {
        try {
            if (req.body.empty()) {
                return setJsonError(res, "empty body");
            }

            // 1) Body → Bytes
            std::vector<uint8_t> buf(req.body.begin(), req.body.end());

            // 2) Deserialisieren
            Block block = Block::deserialize(buf);

            // 3) Header-Signatur prüfen
            if (!verifyBlockHeaderSignature(block.header)) {
                return setJsonError(res, "invalid block header signature", 400);
            }

            // 4) prevHash & Höhe gegen aktuelle Chain prüfen
            const auto& chainVec = chain_.getChain();

            std::array<uint8_t, 32> expectedPrev{};
            uint64_t expectedHeight = 0;

            if (!chainVec.empty()) {
                const Block& last = chainVec.back();
                expectedPrev      = last.hash();
                expectedHeight    = last.header.height + 1;

                if (block.header.prevHash != expectedPrev) {
                    return setJsonError(res, "prevHash mismatch", 400);
                }
            } else {
                // Kette leer → wir erwarten Genesis-Block (height 0)
                expectedHeight = 0;
                // prevHash kann z.B. alles 0 sein – dein Genesis-Format entscheidet.
            }

            if (block.header.height != expectedHeight) {
                return setJsonError(
                    res,
                    "height mismatch: got " + std::to_string(block.header.height) +
                    ", expected " + std::to_string(expectedHeight),
                    400
                );
            }

            // 5) In Blockchain einfügen
            // Blockchain::appendBlock() validiert PoA, Merkle-Root, Timestamp
            if (!chain_.appendBlock(block)) {
                return setJsonError(res, "appendBlock failed", 400);
            }

            // 6) Optional: an Peers broadcasten
            if (peers_) {
                // Block in Message verpacken
                Message msg;
                msg.type    = MessageType::BLOCK;
                msg.payload = block.serialize(); // gleicher Format wie Request-Body

                peers_->broadcast(msg);
            }

            json out;
            out["ok"]      = true;
            out["height"]  = block.header.height;
            out["txCount"] = block.transactions.size();
            res.status     = 200;
            res.set_content(out.dump(2), "application/json");

        } catch (const std::exception& ex) {
            return setJsonError(res, std::string("exception: ") + ex.what(), 500);
        } catch (...) {
            return setJsonError(res, "unknown error", 500);
        }
    });

    // ------------------------------------------------------
    // Server starten
    // ------------------------------------------------------
    std::cout << "[GossipServer] Listening on port " << port_
              << " (/ping, /gossip/tx, /gossip/block)\n";
    server.listen("0.0.0.0", port_);
}


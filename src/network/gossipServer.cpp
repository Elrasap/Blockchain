// src/network/gossipServer.cpp
#include "network/gossipServer.hpp"

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>

#include "core/block.hpp"
#include "core/transaction.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTxValidator.hpp"
#include "dnd/dndPayload.hpp"   // falls isDndPayload dort steckt

using json = nlohmann::json;

// ---------------------------------------------------------
// Kleine Helfer für JSON-Antworten
// ---------------------------------------------------------

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

static void setJsonOk(httplib::Response& res,
                      const json& payload = json::object())
{
    json j = payload;
    j["ok"] = true;
    res.status = 200;
    res.set_content(j.dump(2), "application/json");
}

// ---------------------------------------------------------
// Konstruktor
// ---------------------------------------------------------

GossipServer::GossipServer(int port,
                           Blockchain& chain,
                           Mempool& mempool,
                           PeerManager* peers,
                           dnd::DndTxValidator* validator)
    : port_(port)
    , chain_(chain)
    , mempool_(mempool)
    , peers_(peers)
    , validator_(validator)
{}

// ---------------------------------------------------------
// start() – HTTP-Server mit /ping, /gossip/tx, /gossip/block
// ---------------------------------------------------------

void GossipServer::start()
{
    httplib::Server server;

    // -----------------------------
    // /ping
    // -----------------------------
    server.Get("/ping", [](const httplib::Request&, httplib::Response& res) {
        json j;
        j["pong"] = true;
        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });

    // -----------------------------
    // /gossip/tx
    //
    // Erwartet:
    //   A) JSON mit kompletter Transaction:
    //      {
    //        "payload": [..bytes..],
    //        "senderPubKey": [..bytes..],
    //        "signature": [..bytes..]
    //      }
    //
    //   ODER
    //
    //   B) JSON mit DnD-Event:
    //      {
    //        "encounterId": "...",
    //        "actorId": "...",
    //        "actorType": 0/1,
    //        "targetId": "...",
    //        "targetType": 0/1,
    //        "roll": 0,
    //        "damage": 0,
    //        "hit": true/false,
    //        "note": "...",
    //        "timestamp": <optional>,
    //        "senderPubKey": [...],
    //        "signature": [...]
    //      }
    //
    // In beiden Fällen:
    //   - TX wird validiert (Signatur + ggf. DnD-Validator im Mempool)
    //   - in den Mempool gelegt (addTransactionValidated)
    //   - via PeerManager broadcastet
    // -----------------------------
    server.Post("/gossip/tx",
                [this](const httplib::Request& req, httplib::Response& res) {
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
            // --------- Fall A: Komplette Transaction via JSON ---------
            try {
                tx.payload      = in.at("payload").get<std::vector<uint8_t>>();
                tx.senderPubkey = in.at("senderPubKey").get<std::vector<uint8_t>>();
                tx.signature    = in.at("signature").get<std::vector<uint8_t>>();
            } catch (...) {
                return setJsonError(res, "invalid transaction JSON format");
            }
        } else {
            // --------- Fall B: Reiner DnD-Event (als JSON) ---------
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

            // Wenn kein Timestamp gesetzt wurde → jetzt
            if (in.contains("timestamp")) {
                evt.timestamp = in["timestamp"].get<uint64_t>();
            } else {
                evt.timestamp = static_cast<uint64_t>(time(nullptr));
            }

            if (in.contains("senderPubKey")) {
                evt.senderPubKey =
                    in["senderPubKey"].get<std::vector<uint8_t>>();
            }
            if (in.contains("signature")) {
                evt.signature =
                    in["signature"].get<std::vector<uint8_t>>();
            }

            // Optional: DnD-Regelvalidierung (falls Validator vorhanden)
            if (validator_) {
                std::string vErr;
                if (!validator_->validate(evt, vErr)) {
                    return setJsonError(res,
                                        std::string("DnD validation failed: ")
                                        + vErr,
                                        400);
                }
            }

            // In eine Transaction wrappen
            tx.payload      = dnd::encodeDndTx(evt);
            tx.senderPubkey = evt.senderPubKey;
            tx.signature    = evt.signature;
        }

        // ------ In den Mempool (inkl. Signaturprüfungen etc.) ------
        std::string err;
        if (!mempool_.addTransactionValidated(tx, err)) {
            return setJsonError(res,
                                std::string("mempool rejected tx: ") + err,
                                400);
        }

        // Optional: an alle Peers broadcasten
        if (peers_) {
            peers_->broadcastTransaction(tx);
        }

        json out;
        out["txHash"] = toString(tx.hash()); // toString aus testFramework / util?
        setJsonOk(res, out);
    });

    // -----------------------------
    // /gossip/block
    //
    // Erwartet:
    //   - HTTP-Body = Block::serialize() (rohe Bytes)
    //
    // Schritte:
    //   1. Body → std::vector<uint8_t>
    //   2. Block::deserialize
    //   3. Header-Signatur prüfen
    //   4. prevHash & height gegen lokale Chain prüfen
    //   5. chain_.appendBlock(block)
    //   6. OPTIONAL: an Peers broadcasten
    //
    // DnDState-Update passiert in Blockchain::appendBlock() bereits.
    // -----------------------------
    server.Post("/gossip/block",
                [this](const httplib::Request& req, httplib::Response& res) {
        if (req.body.empty()) {
            return setJsonError(res, "empty body");
        }

        // 1) Body → Bytes
        std::vector<uint8_t> buf(req.body.begin(), req.body.end());

        // 2) Deserialisieren
        Block block = Block::deserialize(buf);

        // 3) Header-Signatur prüfen (PoA)
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
            // Leere Chain → wir erwarten Genesis (height 0)
            expectedHeight = 0;
            // prevHash kann z.B. 0...0 sein (wie bei deinem Genesis)
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
        if (!chain_.appendBlock(block)) {
            return setJsonError(res, "appendBlock failed (PoA / merkle / time)", 400);
        }

        // 6) Optional: an Peers broadcasten
        if (peers_) {
            peers_->broadcastBlock(block);
        }

        json out;
        out["height"]  = block.header.height;
        out["txCount"] = block.transactions.size();
        setJsonOk(res, out);
    });

    // -----------------------------
    // Server starten
    // -----------------------------
    std::cout << "[GossipServer] Listening on port " << port_
              << " (/ping, /gossip/tx, /gossip/block)\n";
    server.listen("0.0.0.0", port_);
}


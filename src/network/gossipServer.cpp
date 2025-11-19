#include "network/gossipServer.hpp"
#include "util/hex.hpp"

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>

#include "core/block.hpp"
#include "core/transaction.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTxValidator.hpp"
#include "dnd/dndPayload.hpp"

using json = nlohmann::json;

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

void GossipServer::start()
{
    running = true;

    // /ping
    server.Get("/ping", [](const httplib::Request&, httplib::Response& res) {
        json j;
        j["pong"] = true;
        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });

    // /gossip/tx
    server.Post("/gossip/tx",
                [this](const httplib::Request& req, httplib::Response& res) {

        if (req.body.empty())
            return setJsonError(res, "empty body");

        json in;
        try { in = json::parse(req.body); }
        catch(...) { return setJsonError(res, "invalid JSON"); }

        Transaction tx;

        bool fullTx =
            in.contains("payload") &&
            in.contains("senderPubKey") &&
            in.contains("signature");

        if (fullTx) {
            try {
                tx.payload      = in["payload"].get<std::vector<uint8_t>>();
                tx.senderPubkey = in["senderPubKey"].get<std::vector<uint8_t>>();
                tx.signature    = in["signature"].get<std::vector<uint8_t>>();
            } catch(...) {
                return setJsonError(res, "invalid tx format");
            }
        } else {
            dnd::DndEventTx evt;

            evt.encounterId = in.value("encounterId", "");
            evt.actorId     = in.value("actorId", "");
            evt.targetId    = in.value("targetId", "");

            evt.actorType  = in.value("actorType", 0);
            evt.targetType = in.value("targetType", 0);

            evt.roll   = in.value("roll", 0);
            evt.damage = in.value("damage", 0);
            evt.hit    = in.value("hit", false);

            if (in.contains("timestamp"))
                evt.timestamp = in["timestamp"].get<uint64_t>();
            else
                evt.timestamp = time(nullptr);

            if (in.contains("senderPubKey"))
                evt.senderPubKey = in["senderPubKey"].get<std::vector<uint8_t>>();

            if (in.contains("signature"))
                evt.signature = in["signature"].get<std::vector<uint8_t>>();

            if (validator_) {
                std::string err;
                if (!validator_->validate(evt, err))
                    return setJsonError(res, "DnD validation failed: " + err);
            }

            tx.payload      = dnd::encodeDndTx(evt);
            tx.senderPubkey = evt.senderPubKey;
            tx.signature    = evt.signature;
        }

        // → Mempool
        std::string err;
        if (!mempool_.addTransactionValidated(tx, err))
            return setJsonError(res, "mempool rejected tx: " + err);

        if (peers_)
            peers_->broadcastTransaction(tx);

        json out;
        out["txHash"] = util::toHex(tx.hash());
        setJsonOk(res, out);
    });

    // /gossip/block
    server.Post("/gossip/block",
                [this](const httplib::Request& req, httplib::Response& res) {

        if (req.body.empty())
            return setJsonError(res, "empty body");

        std::vector<uint8_t> buf(req.body.begin(), req.body.end());

        Block block = Block::deserialize(buf);

        if (!verifyBlockHeaderSignature(block.header))
            return setJsonError(res, "invalid block header signature");

        const auto& chainVec = chain_.getChain();

        uint64_t expectedHeight = 0;
        std::array<uint8_t,32> expectedPrev{};

        if (!chainVec.empty()) {
            const Block& last = chainVec.back();
            expectedPrev   = last.hash();
            expectedHeight = last.header.height + 1;

            if (block.header.prevHash != expectedPrev)
                return setJsonError(res, "prevHash mismatch");
        }

        if (block.header.height != expectedHeight)
            return setJsonError(res, "height mismatch");

        if (!chain_.appendBlock(block))
            return setJsonError(res, "appendBlock failed");

        if (peers_)
            peers_->broadcastBlock(block);

        json out;
        out["height"]  = block.header.height;
        out["txCount"] = block.transactions.size();
        setJsonOk(res, out);
    });

    std::cout << "[GossipServer] Listening on port "
              << port_ << std::endl;

    server.listen("0.0.0.0", port_);
}

// ---------------------------------------------------------

void GossipServer::stop()
{
    if (!running)
        return;

    running = false;

    server.stop();
}


#include "network/gossipServer.hpp"
#include "util/hex.hpp"

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>
#include <iostream>

#include "core/block.hpp"
#include "core/transaction.hpp"

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
                           PeerManager* peers)
    : port_(port)
    , chain_(chain)
    , mempool_(mempool)
    , peers_(peers)
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
        if (req.body.size() > Block::MAX_BLOCK_SIZE * 2)
            return setJsonError(res, "request body too large", 413);

        json in;
        try { in = json::parse(req.body); }
        catch(...) { return setJsonError(res, "invalid JSON"); }

        Transaction tx;
        if (in.contains("wire")) {
            try {
                std::string error;
                if (!Transaction::deserialize(
                        in["wire"].get<std::vector<uint8_t>>(), tx, error))
                    return setJsonError(res, error);
            } catch (...) {
                return setJsonError(res, "invalid tx format");
            }
        } else {
            try {
                tx.payload = in.at("payload").get<std::vector<uint8_t>>();
                tx.senderPubkey = in.at("senderPubKey").get<std::vector<uint8_t>>();
                tx.signature = in.at("signature").get<std::vector<uint8_t>>();
                tx.nonce = in.value("nonce", uint64_t{0});
                tx.fee = in.value("fee", uint64_t{0});
            } catch (...) {
                return setJsonError(res, "complete signed transaction required");
            }
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
        if (req.body.size() > Block::MAX_BLOCK_SIZE)
            return setJsonError(res, "block too large", 413);

        std::vector<uint8_t> buf(req.body.begin(), req.body.end());

        Block block;
        std::string decodeError;
        if (!Block::deserialize(buf, block, decodeError))
            return setJsonError(res, decodeError);

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

        mempool_.remove(block.transactions);

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

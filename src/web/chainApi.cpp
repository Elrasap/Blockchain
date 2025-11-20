#include "web/chainApi.hpp"
#include <nlohmann/json.hpp>
#include "core/blockchain.hpp"
#include "core/crypto.hpp"

using json = nlohmann::json;

ChainApi::ChainApi(Blockchain& chain)
    : chain_(chain) {}

static json blockToJson(const Block& b)
{
    json j;

    j["height"] = b.header.height;
    j["timestamp"] = b.header.timestamp;

    auto h = b.header.hash();
    j["hash"] = crypto::toHex(h);
    j["prevHash"] = crypto::toHex(b.header.prevHash);

    j["txCount"] = b.transactions.size();
    j["transactions"] = json::array();

    for (const auto& tx : b.transactions) {
        json t;
        t["size"] = tx.payload.size();
        j["transactions"].push_back(t);
    }

    return j;
}

void ChainApi::bind(httplib::Server& server)
{
    // -------------------------------
    // GET /chain/height
    // -------------------------------
    server.Get("/chain/height", [&](const httplib::Request&, httplib::Response& res) {
        json j;
        j["height"] = chain_.getHeight();
        res.set_content(j.dump(2), "application/json");
    });

    // -------------------------------
    // GET /chain/latest
    // -------------------------------
    server.Get("/chain/latest", [&](const httplib::Request&, httplib::Response& res) {
        Block b = chain_.getLatestBlock();
        json j = blockToJson(b);
        res.set_content(j.dump(2), "application/json");
    });

    // -------------------------------
    // GET /chain/block/<n>
    // -------------------------------
    server.Get(R"(/chain/block/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int h = std::stoi(req.matches[1]);

        Block b = chain_.getBlock(h);

        // ======= FIX: block existence check =======
        if (b.header.height != h) {
            res.status = 404;
            res.set_content("{\"error\":\"block not found\"}", "application/json");
            return;
        }

        json j = blockToJson(b);
        res.set_content(j.dump(2), "application/json");
    });

    // -------------------------------
    // GET /chain/status
    // -------------------------------
    server.Get("/chain/status", [&](const httplib::Request&, httplib::Response& res) {
        json j;

        j["height"] = chain_.getHeight();
        j["latest"] = blockToJson(chain_.getLatestBlock());

        res.set_content(j.dump(2), "application/json");
    });

    // -------------------------------
    // GET /chain/peers (placeholder)
    // -------------------------------
    server.Get("/chain/peers", [&](const httplib::Request&, httplib::Response& res) {
        json j;
        j["peers"] = json::array();
        res.set_content(j.dump(2), "application/json");
    });
}


#include "web/dmApi.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;
using namespace std;

namespace web {

DmApi::DmApi(Blockchain& chain,
             Mempool& mem,
             const vector<uint8_t>& dmPriv,
             const vector<uint8_t>& dmPub)
    : chain_(chain), mempool_(mem), dmPriv_(dmPriv), dmPub_(dmPub)
{
}

void DmApi::registerRoutes(httplib::Server& server)
{
    // ============================================================
    //   GET /dnd/api/mempool
    // ============================================================
    server.Get("/dnd/api/mempool", [&](const httplib::Request& req,
                                       httplib::Response& res)
    {
        json j;
        j["pending"] = json::array();

        auto txs = mempool_.getAll();
        for (const auto& tx : txs) {
            json e;
            e["senderPubKey"] = tx.senderPubkey;
            e["payloadSize"]  = tx.payload.size();
            e["nonce"]        = tx.nonce;
            j["pending"].push_back(e);
        }

        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });


    // ============================================================
    //   POST /dnd/api/block/mine
    // ============================================================
    server.Post("/dnd/api/block/mine", [&](const httplib::Request& req,
                                           httplib::Response& res)
    {
        // --- Build block ---
        BlockBuilder builder(chain_, dmPriv_, dmPub_);
        auto txs = mempool_.getAll();

        if (txs.empty()) {
            json err = {{"error", "mempool is empty"}};
            res.status = 400;
            res.set_content(err.dump(), "application/json");
            return;
        }

        Block block = builder.buildBlock(txs);

        // --- Append to chain ---
        if (!chain_.appendBlock(block)) {
            json err = {{"error", "block validation failed"}};
            res.status = 400;
            res.set_content(err.dump(), "application/json");
            return;
        }

        // --- Clear mempool ---
        mempool_.clear();

        json ok = {
            {"status", "ok"},
            {"height", block.header.height},
            {"tx_count", txs.size()}
        };

        res.status = 200;
        res.set_content(ok.dump(2), "application/json");
    });
}

} // namespace web


#include "web/chainApi.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ChainApi::ChainApi(Blockchain& chain)
    : chain_(chain)
{
}

std::string ChainApi::hashToHex(const std::array<uint8_t, 32>& h) const
{
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (auto b : h) {
        out.push_back(hex[(b >> 4) & 0xF]);
        out.push_back(hex[b & 0xF]);
    }
    return out;
}

void ChainApi::bind(httplib::Server& svr)
{
    // /chain/height (falls noch nicht vorhanden, ansonsten egal)
    svr.Get("/chain/height", [this](const httplib::Request&, httplib::Response& res) {
        json j;
        j["height"] = chain_.getHeight();
        res.set_content(j.dump(2), "application/json");
    });

    // /chain/latest
    svr.Get("/chain/latest", [this](const httplib::Request&, httplib::Response& res) {
        auto blk = chain_.getLatestBlock();
        json j;

        j["height"]    = blk.header.height;
        j["timestamp"] = blk.header.timestamp;
        j["hash"]      = hashToHex(blk.hash());
        j["prevHash"]  = hashToHex(blk.header.prevHash);
        j["txCount"]   = blk.transactions.size();

        res.set_content(j.dump(2), "application/json");
    });

    // /chain/range?from=..&to=..
    svr.Get("/chain/range", [this](const httplib::Request& req,
                                   httplib::Response& res) {
        uint64_t from = 0;
        uint64_t to   = chain_.getHeight();

        if (auto it = req.get_param_value("from"); !it.empty()) {
            from = std::stoull(it);
        }
        if (auto it = req.get_param_value("to"); !it.empty()) {
            to = std::stoull(it);
        }

        json arr = json::array();

        for (uint64_t h = from; h <= to; ++h) {
            auto blk = chain_.getBlock(h);
            if (blk.header.timestamp == 0 && blk.header.height == 0 &&
                blk.header.merkleRoot == std::array<uint8_t, 32>{}) {
                continue; // non-existent
            }

            json j;
            j["height"]    = blk.header.height;
            j["timestamp"] = blk.header.timestamp;
            j["hash"]      = hashToHex(blk.hash());
            j["prevHash"]  = hashToHex(blk.header.prevHash);
            j["txCount"]   = blk.transactions.size();
            arr.push_back(j);
        }

        res.set_content(arr.dump(2), "application/json");
    });

    // /chain/tx/<hash>
    svr.Get(R"(/chain/tx/(\w+))", [this](const httplib::Request& req,
                                         httplib::Response& res) {
        if (req.matches.size() < 2) {
            res.status = 400;
            res.set_content(R"({"error":"missing hash"})", "application/json");
            return;
        }

        std::string wantedHex = req.matches[1];

        for (uint64_t h = 0; h <= chain_.getHeight(); ++h) {
            auto blk = chain_.getBlock(h);
            for (const auto& tx : blk.transactions) {
                auto txh = tx.hash();
                std::string hex = hashToHex(txh);
                if (hex == wantedHex) {
                    json j;
                    j["blockHeight"] = h;
                    j["hash"]        = hex;
                    j["payload"]     = tx.payload;
                    j["senderPubKey"]= tx.senderPubkey;
                    j["signature"]   = tx.signature;

                    res.set_content(j.dump(2), "application/json");
                    return;
                }
            }
        }

        res.status = 404;
        res.set_content(R"({"error":"tx not found"})", "application/json");
    });
}


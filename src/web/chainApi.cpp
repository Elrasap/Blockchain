#include "web/chainApi.hpp"

#include <ctime>
#include <nlohmann/json.hpp>

#include "thirdparty/httplib.h"
#include "core/blockJson.hpp"
#include "core/blockchain.hpp"
#include "network/peerManager.hpp"

using json = nlohmann::json;

httplib::Response ChainApi::jsonOK(const json& j)
{
    httplib::Response r;
    r.status = 200;
    r.set_content(j.dump(2), "application/json");
    return r;
}

httplib::Response ChainApi::jsonErr(const std::string& msg, int status)
{
    httplib::Response r;
    r.status = status;
    r.set_content(json({
        {"ok", false},
        {"error", msg}
    }).dump(2), "application/json");
    return r;
}

ChainApi::ChainApi(Blockchain& chain, PeerManager* peers)
    : chain_(chain), peers_(peers)
{
}

void ChainApi::bind(httplib::Server& server)
{
    // --------------------------------------------------------
    server.Get("/ping", [&](const httplib::Request&, httplib::Response& res) {
        res = jsonOK({{"pong", true}});
    });

    // --------------------------------------------------------
    server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        json j;
        j["ok"]        = true;
        j["height"]    = chain_.getHeight();
        j["timestamp"] = std::time(nullptr);
        res = jsonOK(j);
    });

    // --------------------------------------------------------
    server.Get("/chain/height", [&](const httplib::Request&, httplib::Response& res) {
        res = jsonOK({{"height", chain_.getHeight()}});
    });

    // --------------------------------------------------------
    server.Get("/chain/latest", [&](const httplib::Request&, httplib::Response& res) {
        Block b = chain_.getLatestBlock();
        json j = b;   // <-- to_json(Block) wird automatisch genutzt
        res = jsonOK(j);
    });

    // --------------------------------------------------------
    server.Get(R"(/chain/block/(\d+))",
        [&](const httplib::Request& req, httplib::Response& res)
    {
        int h = std::stoi(req.matches[1]);

        if (h < 0 || (uint64_t)h > chain_.getHeight()) {
            res = jsonErr("invalid height", 400);
            return;
        }

        Block b = chain_.getBlock(h);
        json j = b;
        res = jsonOK(j);
    });

    // --------------------------------------------------------
    server.Get("/chain/peers", [&](const httplib::Request&, httplib::Response& res)
    {
        json arr = json::array();

        if (peers_) {
            for (const auto& p : peers_->getPeers()) {
                json j;
                j["address"]  = p.address;
                j["height"]   = p.height;
                j["lastSeen"] = p.lastSeen;
                arr.push_back(j);
            }
        }

        res = jsonOK({{"peers", arr}});
    });

    // --------------------------------------------------------
    server.Get("/node/info", [&](const httplib::Request&, httplib::Response& res)
    {
        json j;
        j["version"]   = "1.0";
        j["height"]    = chain_.getHeight();
        j["timestamp"] = std::time(nullptr);
        res = jsonOK(j);
    });
}


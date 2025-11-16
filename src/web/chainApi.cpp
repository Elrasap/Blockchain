#include "web/chainApi.hpp"
#include <nlohmann/json.hpp>

using nlohmann::json;
using namespace std;

namespace web {

ChainApi::ChainApi(Blockchain& chain, dnd::DndState& dnd)
    : chain_(chain), dndState_(dnd)
{
}

void ChainApi::registerRoutes(httplib::Server& server)
{
    // ============================================================
    //  GET /chain/height
    // ============================================================
    server.Get("/chain/height", [&](const httplib::Request& req,
                                    httplib::Response& res)
    {
        json j = {
            {"height", chain_.getHeight()}
        };

        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });


    // ============================================================
    //  GET /chain/block/<n>
    // ============================================================
    server.Get(R"(/chain/block/(\d+))", [&](const httplib::Request& req,
                                            httplib::Response& res)
    {
        uint64_t height = std::stoull(req.matches[1].str());

        Block b = chain_.getBlock(height);
        if (b.header.height != height) {
            res.status = 404;
            res.set_content("{\"error\":\"block not found\"}", "application/json");
            return;
        }

        json j;
        j["height"] = b.header.height;
        j["timestamp"] = b.header.timestamp;
        j["prevHash"] = b.header.prevHash;
        j["merkleRoot"] = b.header.merkleRoot;
        j["validatorPubKey"] = b.header.validatorPubKey;

        j["transactions"] = json::array();
        for (const auto& tx : b.transactions) {
            json t;
            t["senderPubKey"] = tx.senderPubkey;
            t["payloadSize"]  = tx.payload.size();
            j["transactions"].push_back(t);
        }

        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });


    // ============================================================
    // GET /dnd/state
    // ============================================================
    server.Get("/dnd/state", [&](const httplib::Request&, httplib::Response& res)
    {
        json j;

        // CHARACTERS
        j["characters"] = json::array();
        for (auto& [id, c] : dndState_.characters)
        {
            json cj;
            cj["id"] = id;
            cj["name"] = c.name;
            cj["hp"] = c.hp;
            cj["ownerPubKey"] = c.ownerPubKey;
            j["characters"].push_back(cj);
        }

        // MONSTERS
        j["monsters"] = json::array();
        for (auto& [id, m] : dndState_.monsters)
        {
            json mj;
            mj["id"] = id;
            mj["name"] = m.name;
            mj["hp"] = m.hp;
            j["monsters"].push_back(mj);
        }

        // ENCOUNTERS
        j["encounters"] = json::array();
        for (auto& [id, e] : dndState_.encounters)
        {
            json ej;
            ej["id"] = id;
            ej["active"] = e.active;
            ej["round"] = e.round;

            ej["actors"] = json::array();
            for (auto& a : e.actors)
            {
                json aj;
                aj["id"]   = a.id;
                aj["kind"] = a.kind;
                ej["actors"].push_back(aj);
            }

            j["encounters"].push_back(ej);
        }

        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });


    // ============================================================
    // GET /dnd/encounter/<id>
    // ============================================================
    server.Get(R"(/dnd/encounter/(.+))", [&](const httplib::Request& req,
                                              httplib::Response& res)
    {
        std::string encId = req.matches[1].str();

        auto it = dndState_.encounters.find(encId);
        if (it == dndState_.encounters.end()) {
            res.status = 404;
            res.set_content("{\"error\":\"encounter not found\"}", "application/json");
            return;
        }

        const auto& e = it->second;

        json j;
        j["id"] = e.id;
        j["active"] = e.active;
        j["round"] = e.round;

        j["actors"] = json::array();
        for (const auto& a : e.actors) {
            json aj;
            aj["id"] = a.id;
            aj["kind"] = a.kind;
            j["actors"].push_back(aj);
        }

        res.status = 200;
        res.set_content(j.dump(2), "application/json");
    });
}

} // namespace web


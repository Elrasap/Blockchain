#include "dnd/dndState.hpp"
#include "dnd/dndTxSerialization.hpp" // falls du dndTxToJson hast

namespace dnd {

// ... dein bisheriger DndApi-Code oben ...

httplib::Response DndApi::getEncounterHistory(const std::string& encId)
{
    const dnd::DndState& st = chain_.getDndState();

    auto it = st.encounters.find(encId);
    if (it == st.encounters.end()) {
        return jsonError("encounter not found", 404);
    }

    const auto& enc = it->second;
    json out;
    out["encounterId"] = enc.id;
    out["active"]      = enc.active;
    out["round"]       = enc.round;
    out["turnIndex"]   = enc.turnIndex;

    json evArr = json::array();
    for (const auto& e : enc.events) {
        json ev;
        ev["encounterId"] = e.encounterId;
        ev["actorId"]     = e.actorId;
        ev["targetId"]    = e.targetId;
        ev["actorType"]   = e.actorType;
        ev["targetType"]  = e.targetType;
        ev["roll"]        = e.roll;
        ev["damage"]      = e.damage;
        ev["hit"]         = e.hit;
        ev["note"]        = e.note;
        ev["timestamp"]   = e.timestamp;
        evArr.push_back(ev);
    }

    out["events"] = evArr;
    return jsonOK(out);
}

// In install():
// ganz unten in DndApi::install(httplib::Server& server)

void DndApi::install(httplib::Server& server)
{
    auto handle = [&](const httplib::Request& req,
                      httplib::Response& res,
                      const std::string& endpointName)
    {
        dnd::DndEventTx evt;
        std::string err;

        if (!parseJsonToEvent(req, evt, err)) {
            res = jsonError(endpointName + ": " + err, 400);
            return;
        }

        if (!wrapAndInsert(evt, err)) {
            res = jsonError(endpointName + ": " + err, 400);
            return;
        }

        res = jsonOK({{"event", endpointName}});
    };

    // ... deine bereits vorhandenen POST-Endpunkte ...

    server.Post("/dnd/endEncounter",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "endEncounter");
                });

    // NEU: GET /dnd/history/<encId>
    server.Get(R"(/dnd/history/(\w+))",
               [this](const httplib::Request& req,
                      httplib::Response& res) {
                   if (req.matches.size() < 2) {
                       res = jsonError("missing encounterId", 400);
                       return;
                   }
                   std::string encId = req.matches[1];
                   res = getEncounterHistory(encId);
               });
}

} // namespace dnd


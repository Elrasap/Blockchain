// ==========================================================
// dndApi.cpp — komplette fehlerfreie Version
// ==========================================================

#include "web/dndApi.hpp"

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>

#include "core/blockchain.hpp"
#include "core/transaction.hpp"
#include "core/mempool.hpp"

#include "network/peerManager.hpp"

#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndPayload.hpp"
#include "dnd/dndState.hpp"

using json = nlohmann::json;

namespace dnd {

// -----------------------------------------------------------
// Constructor
// -----------------------------------------------------------
DndApi::DndApi(Blockchain& chain,
               Mempool& mempool,
               PeerManager* peers,
               dnd::DndTxValidator& validator,
               const std::vector<uint8_t>& dmPriv,
               const std::vector<uint8_t>& dmPub)
    : chain_(chain),
      mempool_(mempool),
      peers_(peers),
      validator_(validator),
      dmPriv_(dmPriv),
      dmPub_(dmPub)
{}

// -----------------------------------------------------------
// JSON helpers
// -----------------------------------------------------------
httplib::Response DndApi::jsonError(const std::string& msg, int status)
{
    httplib::Response r;
    r.status = status;
    r.set_content(json({{"ok", false}, {"error", msg}}).dump(2),
                  "application/json");
    return r;
}

httplib::Response DndApi::jsonOK(const json& data)
{
    json out = data;
    out["ok"] = true;

    httplib::Response r;
    r.status = 200;
    r.set_content(out.dump(2), "application/json");
    return r;
}

// -----------------------------------------------------------
// A) Parse JSON → DndEventTx
// -----------------------------------------------------------
bool DndApi::parseJsonToEvent(const httplib::Request& req,
                              DndEventTx& evt,
                              std::string& errOut)
{
    json j;
    try {
        j = json::parse(req.body);
    } catch (...) {
        errOut = "invalid JSON";
        return false;
    }

    if (!j.contains("encounterId")) {
        errOut = "missing encounterId";
        return false;
    }
    if (!j.contains("actorId")) {
        errOut = "missing actorId";
        return false;
    }

    evt.encounterId = j.value("encounterId", "");
    evt.actorId     = j.value("actorId", "");
    evt.targetId    = j.value("targetId", "");

    evt.actorType   = j.value("actorType", 0);
    evt.targetType  = j.value("targetType", 0);

    evt.roll        = j.value("roll", 0);
    evt.damage      = j.value("damage", 0);
    evt.hit         = j.value("hit", false);
    evt.note        = j.value("note", "");

    evt.timestamp = (uint64_t)time(nullptr);

    if (j.contains("senderPubKey"))
        evt.senderPubKey = j["senderPubKey"].get<std::vector<uint8_t>>();

    if (j.contains("signature"))
        evt.signature = j["signature"].get<std::vector<uint8_t>>();

    // Auto-sign by DM if no signature present
    if (!j.contains("signature")) {
        evt.senderPubKey = dmPub_;
        dnd::signDndEvent(evt, dmPriv_);
    }

    return true;
}

// -----------------------------------------------------------
// B) Wrap Tx + Validate + Mempool Insert + Broadcast
// -----------------------------------------------------------
bool DndApi::wrapAndInsert(const DndEventTx& evtInput,
                           std::string& errOut)
{
    // 1. TX aufbauen
    Transaction tx;
    tx.payload      = dnd::encodeDndTx(evtInput);
    tx.senderPubkey = evtInput.senderPubKey;
    tx.signature    = evtInput.signature;

    // 2. DnD Payload → Event extrahieren
    dnd::DndEventTx ev;
    try {
        ev = dnd::decodeDndTx(tx.payload);
    } catch (...) {
        errOut = "invalid DnD payload";
        return false;
    }

    // Korrekte Felder übernehmen
    ev.senderPubKey = tx.senderPubkey;
    ev.signature    = tx.signature;

    // 3. VALIDATION (richtige Methode!)
    if (!validator_.validate(ev, errOut)) {
        return false;
    }

    // 4. In Mempool
    if (!mempool_.addTransactionValidated(tx, errOut)) {
        return false;
    }

    // 5. Broadcast an Peers
    if (peers_)
        peers_->broadcastTransaction(tx);

    return true;
}


// -----------------------------------------------------------
// GET /dnd/history/<encId>
// -----------------------------------------------------------
httplib::Response DndApi::getEncounterHistory(const std::string& encId)
{
    const DndState& st = chain_.getDndState();

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

    json events = json::array();
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
        events.push_back(ev);
    }

    out["events"] = events;

    return jsonOK(out);
}

// -----------------------------------------------------------
// Install all endpoints
// -----------------------------------------------------------
void DndApi::install(httplib::Server& server)
{
    auto handle = [&](const httplib::Request& req,
                      httplib::Response& res,
                      const std::string& endpointName)
    {
        DndEventTx evt;
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

    // -------------------------------------------
    // POST Endpoints: Kampfaktionen
    // -------------------------------------------

    server.Post("/dnd/createCharacter",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "createCharacter");
                });

    server.Post("/dnd/spawnMonster",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "spawnMonster");
                });

    server.Post("/dnd/startEncounter",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "startEncounter");
                });

    server.Post("/dnd/initiative",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "initiative");
                });

    server.Post("/dnd/hit",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "hit");
                });

    server.Post("/dnd/damage",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "damage");
                });

    server.Post("/dnd/skillCheck",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "skillCheck");
                });

    server.Post("/dnd/endEncounter",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "endEncounter");
                });

    // -------------------------------------------
    // GET /dnd/history/<encId>
    // -------------------------------------------

    server.Get(R"(/dnd/history/(\w+))",
               [this](const httplib::Request& req,
                      httplib::Response& res)
               {
                   if (req.matches.size() < 2) {
                       res = jsonError("missing encounterId", 400);
                       return;
                   }

                   std::string encId = req.matches[1];
                   res = getEncounterHistory(encId);
               });
}

} // namespace dnd


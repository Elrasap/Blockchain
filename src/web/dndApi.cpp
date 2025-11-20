// dndApi.cpp — robuste Version mit sauberer TX-Signatur
// ===============================================

#include "web/dndApi.hpp"

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <set>
#include <unordered_map>
#include <ctime>

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
// A) Parse JSON → DndEventTx (OHNE Signatur)
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

    evt.timestamp = j.value("timestamp",
                            static_cast<uint64_t>(time(nullptr)));

    // optional: wer "angeblich" handelt
    if (j.contains("senderPubKey")) {
        evt.senderPubKey = j["senderPubKey"].get<std::vector<uint8_t>>();
    } else {
        evt.senderPubKey.clear();
    }

    // WICHTIG:
    // Event-Signaturen werden NICHT mehr benutzt.
    // Die echte Signatur liegt auf der Transaction.
    evt.signature.clear();

    return true;
}

// -----------------------------------------------------------
// B) Wrap, Validate, Insert into Mempool
//    -> Hier wird die Transaction signiert, nicht das Event.
// -----------------------------------------------------------
bool DndApi::wrapAndInsert(const DndEventTx& evtInput,
                           std::string& errOut)
{
    // 1. Payload: reines, binäres DnD-Event (ohne Signatur)
    Transaction tx;
    tx.payload = dnd::encodeDndTx(evtInput);

    // 2. Absender-Pubkey:
    //    - Wenn Client einen senderPubKey setzt → könnte später
    //      für Player-Mode benutzt werden.
    //    - Aktuell: fallback auf DM-Pubkey.
    if (!evtInput.senderPubKey.empty()) {
        tx.senderPubkey = evtInput.senderPubKey;
    } else {
        tx.senderPubkey = dmPub_; // DM als Sender
    }

    // 3. Transaction signieren (nur hier!)
    //    Signatur liegt NUR auf der TX, nicht im Event.
    tx.sign(dmPriv_);

    // 4. DnD-Event aus Payload extrahieren, um zu validieren
    dnd::DndEventTx ev;
    try {
        ev = dnd::decodeDndTx(tx.payload);
    } catch (const std::exception& e) {
        errOut = std::string("invalid DnD payload: ") + e.what();
        return false;
    } catch (...) {
        errOut = "invalid DnD payload";
        return false;
    }

    // Signatur-Metadaten für Validator:
    ev.senderPubKey = tx.senderPubkey;
    ev.signature.clear(); // wird nicht mehr ausgewertet

    // 5. Semantic Validation (DnD-Regeln)
    if (!validator_.validate(ev, errOut)) {
        return false;
    }

    // 6. In Mempool
    if (!mempool_.addTransactionValidated(tx, errOut)) {
        return false;
    }

    // 7. Broadcast an Peers
    if (peers_) {
        peers_->broadcastTransaction(tx);
    }

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
// GET /dnd/state
// -----------------------------------------------------------
httplib::Response DndApi::getState()
{
    const DndState& st = chain_.getDndState();

    std::set<std::string> characterIds;
    std::set<std::string> monsterIds;
    std::unordered_map<std::string, int> dmgChar;
    std::unordered_map<std::string, int> dmgMon;

    for (const auto& [encId, enc] : st.encounters) {
        for (const auto& e : enc.events) {
            if (!e.actorId.empty()) {
                if (e.actorType == 0)
                    characterIds.insert(e.actorId);
                else if (e.actorType == 1)
                    monsterIds.insert(e.actorId);
            }
            if (!e.targetId.empty()) {
                if (e.targetType == 0)
                    characterIds.insert(e.targetId);
                else if (e.targetType == 1)
                    monsterIds.insert(e.targetId);
            }

            if (e.hit && e.damage > 0 && !e.targetId.empty()) {
                if (e.targetType == 0)
                    dmgChar[e.targetId] += e.damage;
                else if (e.targetType == 1)
                    dmgMon[e.targetId] += e.damage;
            }
        }
    }

    json jChars = json::object();
    for (const auto& id : characterIds) {
        int hpNow = st.getCharacterHp(id);
        int dmg   = dmgChar[id];
        int maxHp = hpNow + dmg;
        if (maxHp <= 0) maxHp = 10;

        json c;
        c["id"]     = id;
        c["name"]   = id;
        c["hp"]     = hpNow;
        c["maxHp"]  = maxHp;
        c["ac"]     = 12;
        c["level"]  = 1;

        c["str"] = 14; c["dex"] = 12; c["con"] = 14;
        c["int"] = 10; c["wis"] = 10; c["cha"] = 12;

        jChars[id] = c;
    }

    json jMons = json::object();
    for (const auto& id : monsterIds) {
        int hpNow = st.getMonsterHp(id);
        int dmg   = dmgMon[id];
        int maxHp = hpNow + dmg;
        if (maxHp <= 0) maxHp = 12;

        json m;
        m["id"]    = id;
        m["name"]  = id;
        m["hp"]    = hpNow;
        m["maxHp"] = maxHp;
        m["ac"]    = 11;

        m["str"] = 12; m["dex"] = 11; m["con"] = 12;
        m["int"] = 8;  m["wis"] = 9;  m["cha"] = 8;

        jMons[id] = m;
    }

    json jEncs = json::object();
    for (const auto& [encId, enc] : st.encounters) {
        json e;
        e["id"]        = enc.id;
        e["active"]    = enc.active;
        e["round"]     = enc.round;
        e["turnIndex"] = enc.turnIndex;
        jEncs[encId]   = e;
    }

    json out;
    out["characters"] = jChars;
    out["monsters"]   = jMons;
    out["encounters"] = jEncs;

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
        try {
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
        }
        catch (const std::exception& ex) {
            std::cerr << "[DndApi] Exception in handler '" << endpointName
                      << "': " << ex.what() << "\n";
            res = jsonError("internal error in " + endpointName, 500);
        }
        catch (...) {
            std::cerr << "[DndApi] Unknown exception in handler '"
                      << endpointName << "'\n";
            res = jsonError("internal error in " + endpointName, 500);
        }
    };

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

    server.Get("/dnd/state",
               [this](const httplib::Request&,
                      httplib::Response& res)
               {
                   res = getState();
               });
}

} // namespace dnd


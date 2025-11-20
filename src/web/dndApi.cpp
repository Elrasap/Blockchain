// dndApi.cpp — robuste Version ohne doppeltes Decoding
// ===============================================

#include "web/dndApi.hpp"

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <set>
#include <unordered_map>

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
    std::cerr << "[DndApi] parseJsonToEvent: raw body size="
              << req.body.size() << "\n";
    std::cerr << "[DndApi] parseJsonToEvent: body='"
              << req.body << "'\n";

    json j;
    try {
        j = json::parse(req.body);
    }
    catch (const std::exception& e) {
        std::cerr << "[DndApi] JSON parse exception: " << e.what() << "\n";
        errOut = "invalid JSON";
        return false;
    }
    catch (...) {
        std::cerr << "[DndApi] JSON parse unknown exception\n";
        errOut = "invalid JSON (unknown error)";
        return false;
    }

    std::cerr << "[DndApi] JSON parse OK\n";

    if (!j.contains("encounterId")) {
        errOut = "missing encounterId";
        return false;
    }
    if (!j.contains("actorId")) {
        errOut = "missing actorId";
        return false;
    }

    // LOG jedes Feld
    std::cerr << "[DndApi] encounterId=" << j.value("encounterId", "") << "\n";
    std::cerr << "[DndApi] actorId=" << j.value("actorId", "") << "\n";

    // eventType
    int et = j.value("eventType", 0);
    evt.eventType = static_cast<DndEventType>(et);

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

    if (j.contains("senderPubKey"))
        evt.senderPubKey = j["senderPubKey"].get<std::vector<uint8_t>>();
    else
        evt.senderPubKey.clear();

    // Event signature IMMER leeren
    evt.signature.clear();

    // Fallback auf DM key
    if (evt.senderPubKey.empty()) {
        evt.senderPubKey = dmPub_;
    }

    std::cerr << "[DndApi] parseJsonToEvent DONE\n";
    return true;
}


// -----------------------------------------------------------
// B) DnD-Event validieren, als TX verpacken, signieren, broadcasten
//    KEIN decodeDndTx mehr hier.
// -----------------------------------------------------------
bool DndApi::wrapAndInsert(const DndEventTx& evtInput,
                           std::string& errOut)
{
    try {
        // 1. Lokale Kopie, damit wir evtInput nicht verändern
        DndEventTx evt = evtInput;

        // Fallback: wenn kein senderPubKey -> DM übernimmt
        if (evt.senderPubKey.empty()) {
            evt.senderPubKey = dmPub_;
        }

        // 2. Transaction bauen
        Transaction tx;

        // kompakter DnD-Body (magic 0xD1, eventType etc.)
        tx.payload      = dnd::encodeDndTx(evt);
        tx.senderPubkey = evt.senderPubKey;

        // Signatur: DM signiert die TX
        tx.sign(dmPriv_);

        // 3. In den Mempool (hier KEIN weiterer DnD-Decode mehr)
        if (!mempool_.addTransactionValidated(tx, errOut)) {
            return false;
        }

        // 4. An Peers broadcasten
        if (peers_) {
            peers_->broadcastTransaction(tx);
        }

        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "[DndApi] wrapAndInsert exception: " << ex.what() << "\n";
        errOut = std::string("wrapAndInsert exception: ") + ex.what();
        return false;
    }
    catch (...) {
        std::cerr << "[DndApi] wrapAndInsert unknown exception\n";
        errOut = "wrapAndInsert unknown exception";
        return false;
    }
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

    // -------------------------------
    // Aggregationsstrukturen
    // -------------------------------
    struct Agg {
        int damageDealt = 0;
        int damageTaken = 0;
        int hits        = 0;
        int misses      = 0;
    };

    std::set<std::string> characterIds;
    std::set<std::string> monsterIds;

    std::unordered_map<std::string, Agg> charAgg;
    std::unordered_map<std::string, Agg> monAgg;

    // Encounter-Meta
    std::unordered_map<std::string, int> encEventCount;
    std::unordered_map<std::string, int> encActorCount;

    // -------------------------------
    // Events durchlaufen
    // -------------------------------
    for (const auto& [encId, enc] : st.encounters) {
        // Actor-Anzahl merken
        encActorCount[encId] = static_cast<int>(enc.actors.size());
        encEventCount[encId] = static_cast<int>(enc.events.size());

        for (const auto& e : enc.events) {
            // --- Actor registrieren ---
            if (!e.actorId.empty()) {
                if (e.actorType == 0) {
                    characterIds.insert(e.actorId);
                } else if (e.actorType == 1) {
                    monsterIds.insert(e.actorId);
                }
            }

            // --- Target registrieren ---
            if (!e.targetId.empty()) {
                if (e.targetType == 0) {
                    characterIds.insert(e.targetId);
                } else if (e.targetType == 1) {
                    monsterIds.insert(e.targetId);
                }
            }

            // --- Hits / Misses + DamageDealt ---
            if (!e.actorId.empty()) {
                if (e.actorType == 0) {
                    auto& a = charAgg[e.actorId];
                    if (e.hit) {
                        a.hits++;
                        a.damageDealt += e.damage;
                    } else {
                        a.misses++;
                    }
                } else if (e.actorType == 1) {
                    auto& a = monAgg[e.actorId];
                    if (e.hit) {
                        a.hits++;
                        a.damageDealt += e.damage;
                    } else {
                        a.misses++;
                    }
                }
            }

            // --- DamageTaken für Targets ---
            if (e.hit && e.damage > 0 && !e.targetId.empty()) {
                if (e.targetType == 0) {
                    auto& t = charAgg[e.targetId];
                    t.damageTaken += e.damage;
                } else if (e.targetType == 1) {
                    auto& t = monAgg[e.targetId];
                    t.damageTaken += e.damage;
                }
            }
        }
    }

    // -------------------------------
    // Characters JSON bauen
    // -------------------------------
    json jChars = json::object();
    for (const auto& id : characterIds) {
        int hpNow = st.getCharacterHp(id);

        Agg a{};
        auto it = charAgg.find(id);
        if (it != charAgg.end()) {
            a = it->second;
        }

        int maxHp = hpNow + a.damageTaken;
        if (maxHp <= 0) maxHp = 10;

        json c;
        c["id"]           = id;
        c["name"]         = id;
        c["hp"]           = hpNow;
        c["maxHp"]        = maxHp;

        // Neue Felder
        c["damageTaken"]  = a.damageTaken;
        c["damageDealt"]  = a.damageDealt;
        c["hits"]         = a.hits;
        c["misses"]       = a.misses;

        // Alte Dummy-Attribute zur Kompatibilität
        c["ac"]     = 12;
        c["level"]  = 1;
        c["str"]    = 14; c["dex"] = 12; c["con"] = 14;
        c["int"]    = 10; c["wis"] = 10; c["cha"] = 12;

        jChars[id] = c;
    }

    // -------------------------------
    // Monsters JSON bauen
    // -------------------------------
    json jMons = json::object();
    for (const auto& id : monsterIds) {
        int hpNow = st.getMonsterHp(id);

        Agg a{};
        auto it = monAgg.find(id);
        if (it != monAgg.end()) {
            a = it->second;
        }

        int maxHp = hpNow + a.damageTaken;
        if (maxHp <= 0) maxHp = 12;

        json m;
        m["id"]          = id;
        m["name"]        = id;
        m["hp"]          = hpNow;
        m["maxHp"]       = maxHp;

        // Neue Felder
        m["damageTaken"] = a.damageTaken;
        m["damageDealt"] = a.damageDealt;
        m["hits"]        = a.hits;
        m["misses"]      = a.misses;

        // Dummy-Stats
        m["ac"] = 11;
        m["str"] = 12; m["dex"] = 11; m["con"] = 12;
        m["int"] = 8;  m["wis"] = 9;  m["cha"] = 8;

        jMons[id] = m;
    }

    // -------------------------------
    // Encounter JSON bauen
    // -------------------------------
    json jEncs = json::object();
    for (const auto& [encId, enc] : st.encounters) {
        json e;
        e["id"]        = enc.id;
        e["active"]    = enc.active;
        e["round"]     = enc.round;
        e["turnIndex"] = enc.turnIndex;

        // Neue Felder:
        e["numEvents"] = encEventCount[encId];
        e["numActors"] = encActorCount[encId];

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
                      const std::string& endpointName,
                      DndEventType type)
    {
        try {
            std::cerr << "[DndApi] creating evt struct...\n";

            DndEventTx evt;
            std::cerr << "[DndApi] evt struct constructed OK\n";

            std::string err;

            if (!parseJsonToEvent(req, evt, err)) {
                res = jsonError(endpointName + ": " + err, 400);
                return;
            }

            // ✨ Hier setzen wir den semantischen Typ
            evt.eventType = type;

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
                    handle(req, res, "createCharacter",
                           DndEventType::CreateCharacter);
                });

    server.Post("/dnd/spawnMonster",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "spawnMonster",
                           DndEventType::SpawnMonster);
                });

    server.Post("/dnd/startEncounter",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "startEncounter",
                           DndEventType::StartEncounter);
                });

    server.Post("/dnd/initiative",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "initiative",
                           DndEventType::Initiative);
                });

    server.Post("/dnd/hit",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "hit",
                           DndEventType::Hit);
                });

    server.Post("/dnd/damage",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "damage",
                           DndEventType::Damage);
                });

    server.Post("/dnd/skillCheck",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "skillCheck",
                           DndEventType::SkillCheck);
                });

    server.Post("/dnd/endEncounter",
                [&](const httplib::Request& req, httplib::Response& res) {
                    handle(req, res, "endEncounter",
                           DndEventType::EndEncounter);
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


#pragma once
#include <string>
#include <fstream>
#include "dnd/state.hpp"

#include <nlohmann/json.hpp>
#include "dnd/state.hpp"
#include "dnd/character.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/dndTx.hpp"

namespace dnd {

using nlohmann::json;

// -----------------------------------------------------
// Write the full game state to JSON
// -----------------------------------------------------
inline bool writeSnapshot(const DndState& state,
                          const std::string& path)
{
    json j;

    // ---------- Characters ----------
    {
        json j_chars = json::object();
        for (const auto& [id, cs] : state.characters) {
            j_chars[id] = cs.sheet;  // CharacterSheet hat to_json(...)
        }
        j["characters"] = std::move(j_chars);
    }

    // ---------- Monsters ----------
    {
        json j_mon = json::object();
        for (const auto& [id, m] : state.monsters) {
            json jm;
            jm["id"]    = m.id;
            jm["hp"]    = m.hp;
            jm["maxHp"] = m.maxHp;
            j_mon[id]   = std::move(jm);
        }
        j["monsters"] = std::move(j_mon);
    }

    // ---------- Encounters ----------
    {
        json j_enc = json::object();

        for (const auto& [id, e] : state.encounters) {
            json je;
            je["id"]        = e.id;
            je["active"]    = e.active;
            je["round"]     = e.round;
            je["turnIndex"] = e.turnIndex;

            // actors (CombatActorRef hat to_json in encounter.cpp)
            je["actors"] = e.actors;

            // events → alles explizit reinschreiben
            json j_ev = json::array();
            for (const auto& evt : e.events) {
                json ev;
                ev["encounterId"] = evt.encounterId;
                ev["actorId"]     = evt.actorId;
                ev["targetId"]    = evt.targetId;
                ev["actorType"]   = evt.actorType;
                ev["targetType"]  = evt.targetType;
                ev["roll"]        = evt.roll;
                ev["damage"]      = evt.damage;
                ev["hit"]         = evt.hit;
                ev["note"]        = evt.note;
                ev["timestamp"]   = evt.timestamp;

                // Vektoren gehen als JSON-Array von Zahlen durch
                ev["senderPubKey"] = evt.senderPubKey;
                ev["signature"]    = evt.signature;

                j_ev.push_back(std::move(ev));
            }
            je["events"] = std::move(j_ev);

            j_enc[id] = std::move(je);
        }

        j["encounters"] = std::move(j_enc);
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        return false;
    }

    f << j.dump(2);
    return true;
}

// -----------------------------------------------------
// Read JSON snapshot back into DndState
// -----------------------------------------------------
inline bool loadSnapshot(DndState& state,
                         const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    json j = json::parse(f, nullptr, false);
    if (j.is_discarded()) {
        return false;
    }

    state.characters.clear();
    state.monsters.clear();
    state.encounters.clear();

    // ---------- Characters ----------
    if (j.contains("characters") && j["characters"].is_object()) {
        for (auto& [id, val] : j["characters"].items()) {
            CharacterState cs;
            cs.sheet = val.get<CharacterSheet>();
            state.characters[id] = std::move(cs);
        }
    }

    // ---------- Monsters ----------
    if (j.contains("monsters") && j["monsters"].is_object()) {
        for (auto& [id, val] : j["monsters"].items()) {
            MonsterState m;
            m.id    = id;
            m.hp    = val.value("hp", 0);
            m.maxHp = val.value("maxHp", 0);
            state.monsters[id] = std::move(m);
        }
    }

    // ---------- Encounters ----------
    if (j.contains("encounters") && j["encounters"].is_object()) {
        for (auto& [id, val] : j["encounters"].items()) {
            EncounterState e;
            e.id        = id;
            e.active    = val.value("active", false);
            e.round     = val.value("round", 1);
            e.turnIndex = val.value("turnIndex", 0);

            if (val.contains("actors")) {
                e.actors = val["actors"]
                               .get<std::vector<dnd::combat::CombatActorRef>>();
            }

            // Events manuell aus JSON lesen
            if (val.contains("events") && val["events"].is_array()) {
                for (auto& evVal : val["events"]) {
                    DndEventTx evt;
                    evt.encounterId = evVal.value("encounterId", std::string{});
                    evt.actorId     = evVal.value("actorId", std::string{});
                    evt.targetId    = evVal.value("targetId", std::string{});
                    evt.actorType   = evVal.value("actorType", 0);
                    evt.targetType  = evVal.value("targetType", 0);
                    evt.roll        = evVal.value("roll", 0);
                    evt.damage      = evVal.value("damage", 0);
                    evt.hit         = evVal.value("hit", false);
                    evt.note        = evVal.value("note", std::string{});
                    evt.timestamp   = evVal.value("timestamp", static_cast<uint64_t>(0));

                    if (evVal.contains("senderPubKey")) {
                        evt.senderPubKey =
                            evVal["senderPubKey"].get<std::vector<uint8_t>>();
                    }
                    if (evVal.contains("signature")) {
                        evt.signature =
                            evVal["signature"].get<std::vector<uint8_t>>();
                    }

                    e.events.push_back(std::move(evt));
                }
            }

            state.encounters[id] = std::move(e);
        }
    }

    return true;
}

} // namespace dnd


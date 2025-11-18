#include "dnd/stateSnapshot.hpp"
#include "dnd/dndState.hpp"
#include "dnd/character.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/dndTx.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace dnd {

using json = nlohmann::json;

// ---------------------------------------------------------------------
// JSON helpers for CharacterState / MonsterState / EncounterState
// ---------------------------------------------------------------------

// ---- CharacterState ----
void to_json(json& j, const CharacterState& cs)
{
    j = json{
        {"sheet", cs.sheet}
    };
}

void from_json(const json& j, CharacterState& cs)
{
    cs.sheet = j.at("sheet").get<CharacterSheet>();
}

// ---- MonsterState ----
void to_json(json& j, const MonsterState& m)
{
    j = json{
        {"id",    m.id},
        {"hp",    m.hp},
        {"maxHp", m.maxHp}
    };
}

void from_json(const json& j, MonsterState& m)
{
    m.id    = j.value("id", "");
    m.hp    = j.value("hp", 0);
    m.maxHp = j.value("maxHp", 0);
}

// ---- EncounterState ----
void to_json(json& j, const EncounterState& e)
{
    j = json{
        {"id",        e.id},
        {"active",    e.active},
        {"round",     e.round},
        {"turnIndex", e.turnIndex},
        {"actors",    e.actors},
        {"events",    json::array()}
    };

    for (const auto& evt : e.events)
    {
        json ev = {
            {"encounterId", evt.encounterId},
            {"actorId",     evt.actorId},
            {"targetId",    evt.targetId},
            {"actorType",   evt.actorType},
            {"targetType",  evt.targetType},
            {"roll",        evt.roll},
            {"damage",      evt.damage},
            {"hit",         evt.hit},
            {"note",        evt.note},
            {"timestamp",   evt.timestamp},
            {"senderPubKey", evt.senderPubKey},
            {"signature",    evt.signature}
        };
        j["events"].push_back(ev);
    }
}

void from_json(const json& j, EncounterState& e)
{
    e.id        = j.value("id", "");
    e.active    = j.value("active", false);
    e.round     = j.value("round", 1);
    e.turnIndex = j.value("turnIndex", 0);

    if (j.contains("actors"))
        e.actors = j.at("actors").get<std::vector<dnd::combat::CombatActorRef>>();

    if (j.contains("events"))
    {
        for (const auto& ev : j["events"])
        {
            DndEventTx evt;
            evt.encounterId = ev.value("encounterId", "");
            evt.actorId     = ev.value("actorId", "");
            evt.targetId    = ev.value("targetId", "");
            evt.actorType   = ev.value("actorType", 0);
            evt.targetType  = ev.value("targetType", 0);
            evt.roll        = ev.value("roll", 0);
            evt.damage      = ev.value("damage", 0);
            evt.hit         = ev.value("hit", false);
            evt.note        = ev.value("note", "");
            evt.timestamp   = ev.value("timestamp", (uint64_t)0);

            if (ev.contains("senderPubKey"))
                evt.senderPubKey = ev["senderPubKey"].get<std::vector<uint8_t>>();

            if (ev.contains("signature"))
                evt.signature = ev["signature"].get<std::vector<uint8_t>>();

            e.events.push_back(evt);
        }
    }
}

// ---------------------------------------------------------------------
// Save Snapshot
// ---------------------------------------------------------------------

bool writeSnapshot(const DndState& state, const std::string& path)
{
    json root;

    root["characters"] = state.characters;
    root["monsters"]   = state.monsters;
    root["encounters"] = state.encounters;

    std::ofstream f(path);
    if (!f.is_open())
        return false;

    f << root.dump(2);
    return true;
}

// ---------------------------------------------------------------------
// Load Snapshot
// ---------------------------------------------------------------------

bool loadSnapshot(DndState& state, const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return false;

    json root = json::parse(f, nullptr, false);
    if (root.is_discarded())
        return false;

    state.clear();

    if (root.contains("characters"))
        state.characters = root["characters"].get<decltype(state.characters)>();

    if (root.contains("monsters"))
        state.monsters = root["monsters"].get<decltype(state.monsters)>();

    if (root.contains("encounters"))
        state.encounters = root["encounters"].get<decltype(state.encounters)>();

    return true;
}

} // namespace dnd


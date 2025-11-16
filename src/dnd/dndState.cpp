#include "dnd/dndState.hpp"
#include "core/blockchain.hpp"
#include "core/transaction.hpp"
#include "dnd/serialization.hpp"

using namespace dnd;

#include "dnd/dndState.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace dnd;

/*
===========================================================
 3.4 STATE SNAPSHOTTING
===========================================================
*/

static json snapshotCharacter(const CharacterState& c)
{
    return {
        {"id", c.id},
        {"hpCurrent", c.hpCurrent},
        {"hpMax", c.hpMax},
        {"level", c.level},
        {"name", c.name},
        {"playerAddress", c.playerAddress},
        {"stats", c.stats},
        {"inventory", c.inventory},
        {"conditions", c.conditions}
    };
}

static json snapshotMonster(const MonsterState& m)
{
    return {
        {"id", m.id},
        {"hpCurrent", m.hpCurrent},
        {"hpMax", m.hpMax},
        {"name", m.name},
        {"type", m.type}
    };
}

static json snapshotEncounter(const EncounterState& e)
{
    return {
        {"id", e.id},
        {"name", e.name},
        {"active", e.active},
        {"round", e.round},
        {"turnIndex", e.turnIndex},
        {"log", e.eventLog},
        {"order", e.order}
    };
}


/*
===========================================================
 SAVE SNAPSHOT
===========================================================
*/

bool DndState::saveSnapshot(const std::string& path, std::string& err) const
{
    json j;

    // Characters
    j["characters"] = json::object();
    for (auto& [id, c] : characters)
        j["characters"][id] = snapshotCharacter(c);

    // Monsters
    j["monsters"] = json::object();
    for (auto& [id, m] : monsters)
        j["monsters"][id] = snapshotMonster(m);

    // Encounters
    j["encounters"] = json::object();
    for (auto& [id, e] : encounters)
        j["encounters"][id] = snapshotEncounter(e);

    // Write file
    std::ofstream out(path);
    if (!out) {
        err = "Cannot open snapshot file for writing";
        return false;
    }

    out << j.dump(2);
    return true;
}


/*
===========================================================
 LOAD SNAPSHOT
===========================================================
*/

bool DndState::loadSnapshot(const std::string& path, std::string& err)
{
    std::ifstream in(path);
    if (!in) {
        err = "Snapshot file not found";
        return false;
    }

    json j;
    try {
        in >> j;
    }
    catch (const std::exception& e) {
        err = e.what();
        return false;
    }

    characters.clear();
    monsters.clear();
    encounters.clear();

    // Characters
    if (j.contains("characters"))
    {
        for (auto it = j["characters"].begin(); it != j["characters"].end(); ++it)
        {
            CharacterState c;
            c.id = it.key();
            auto jc = it.value();

            c.name          = jc.value("name", "");
            c.playerAddress = jc.value("playerAddress", "");
            c.hpCurrent     = jc.value("hpCurrent", 0);
            c.hpMax         = jc.value("hpMax", 0);
            c.level         = jc.value("level", 1);
            c.stats         = jc["stats"].get<Stats>();
            c.inventory     = jc["inventory"].get<std::vector<std::string>>();
            c.conditions    = jc["conditions"].get<std::vector<std::string>>();

            characters[c.id] = c;
        }
    }

    // Monsters
    if (j.contains("monsters"))
    {
        for (auto it = j["monsters"].begin(); it != j["monsters"].end(); ++it)
        {
            MonsterState m;
            m.id        = it.key();
            auto jm     = it.value();
            m.name      = jm.value("name", "");
            m.type      = jm.value("type", "");
            m.hpCurrent = jm.value("hpCurrent", 0);
            m.hpMax     = jm.value("hpMax", 0);

            monsters[m.id] = m;
        }
    }

    // Encounters
    if (j.contains("encounters"))
    {
        for (auto it = j["encounters"].begin(); it != j["encounters"].end(); ++it)
        {
            EncounterState e;
            e.id        = it.key();
            auto je     = it.value();
            e.name      = je.value("name", "");
            e.active    = je.value("active", true);
            e.round     = je.value("round", 1);
            e.turnIndex = je.value("turnIndex", 0);

            e.eventLog = je["log"].get<std::vector<DndEventTx>>();

            if (je.contains("order"))
                e.order = je["order"].get<std::vector<std::string>>();

            encounters[e.id] = e;
        }
    }

    return true;
}


bool DndState::rebuildFromChain(const Blockchain& chain, std::string& err)
{
    characters.clear();
    monsters.clear();
    encounters.clear();

    auto blocks = chain.loadChain();

    std::cout << "[DndState] Rebuilding DnD State from "
              << blocks.size() << " blocks...\n";

    for (const auto& block : blocks)
    {
        for (const auto& tx : block.transactions)
        {
            // Nur DnD-Payloads beachten
            if (!dnd::isDndPayload(tx.payload))
                continue;

            // TX extrahieren
            DndEventTx evt = dnd::decodeDndTx(tx.payload);
            evt.senderPubKey = tx.senderPubkey;
            evt.signature    = tx.signature;

            // Signatur prüfen
            if (!dnd::verifyDndEventSignature(evt, err)) {
                std::cerr << "[DndState] ERROR: signature invalid in block "
                          << block.header.height << "\n";
                return false;
            }

            // Logik anwenden
            if (!apply(evt, err)) {
                std::cerr << "[DndState] ERROR applying TX in block "
                          << block.header.height << ": " << err << "\n";
                return false;
            }
        }
    }

    std::cout << "[DndState] Rebuild complete: "
              << characters.size() << " characters, "
              << monsters.size() << " monsters, "
              << encounters.size() << " encounters.\n";

    return true;
}
bool DndState::apply(const DndEventTx& evt, std::string& err)
{
    // ------------------------------------------------------
    // 1. Encounter holen / anlegen
    // ------------------------------------------------------
    auto it = encounters.find(evt.encounterId);
    if (it == encounters.end()) {
        // Encounter automatisch erzeugen (optional)
        EncounterState newEnc;
        newEnc.encounter.id = evt.encounterId;
        newEnc.encounter.active = true;
        newEnc.encounter.round = 1;
        newEnc.encounter.turnIndex = 0;

        encounters[evt.encounterId] = newEnc;
        it = encounters.find(evt.encounterId);
    }

    EncounterState& enc = it->second;

    // ------------------------------------------------------
    // 2. In Log einfügen
    // ------------------------------------------------------
    enc.eventLog.push_back(evt);

    // ------------------------------------------------------
    // 3. Event logischerweise anwenden
    // ------------------------------------------------------
    switch (evt.type)
    {
        case PayloadType::Attack:
            if (!applyAttack(evt, enc, err)) return false;
            break;

        case PayloadType::SkillCheck:
            if (!applySkillCheck(evt, enc, err)) return false;
            break;

        case PayloadType::SavingThrow:
            if (!applySavingThrow(evt, enc, err)) return false;
            break;

        default:
            err = "Unsupported DnD event type";
            return false;
    }

    // ------------------------------------------------------
    // 4. Prüfen: Encounter gewonnen/verloren?
    // ------------------------------------------------------
    // Wenn ALLE Monster HP <= 0 → Encounter finished
    bool allDead = true;

    for (auto& mId : enc.encounter.getMonsterIds()) {
        if (monsters.count(mId) && monsters[mId].hp > 0) {
            allDead = false;
            break;
        }
    }

    if (allDead && !enc.encounter.finished)
    {
        enc.encounter.finished = true;

        // XP verteilen
        int xpReward = 50; // später dynamisch
        for (auto& cRef : enc.encounter.getCharacterRefs())
        {
            if (characters.count(cRef.id)) {
                characters[cRef.id].sheet.xp += xpReward;
            }
        }
    }

    return true;
}

/*
-----------------------------------------------------------
 Attack-Event → HP senken
-----------------------------------------------------------
*/

bool DndState::applyAttack(const DndEventTx& evt,
                           EncounterState& enc,
                           std::string& err)
{
    if (!evt.hit) {
        // Miss → nichts tun
        return true;
    }

    // ------------------------------------------------------
    // Target = Character
    // ------------------------------------------------------
    if (evt.targetType == ActorType::Character)
    {
        auto it = characters.find(evt.targetId);
        if (it == characters.end()) {
            err = "Target character does not exist: " + evt.targetId;
            return false;
        }

        auto& ch = it->second.sheet;
        ch.hpCurrent -= evt.damage;
        if (ch.hpCurrent < 0) ch.hpCurrent = 0;

        return true;
    }

    // ------------------------------------------------------
    // Target = Monster
    // ------------------------------------------------------
    if (evt.targetType == ActorType::Monster)
    {
        auto it = monsters.find(evt.targetId);
        if (it == monsters.end()) {
            err = "Target monster does not exist: " + evt.targetId;
            return false;
        }

        auto& mon = it->second;
        mon.hp -= evt.damage;
        if (mon.hp < 0) mon.hp = 0;

        return true;
    }

    err = "Unknown target type in applyAttack()";
    return false;
}

/*
-----------------------------------------------------------
 Skill Checks → kein Schaden
-----------------------------------------------------------
*/

bool DndState::applySkillCheck(const DndEventTx& evt,
                               EncounterState& enc,
                               std::string& err)
{
    // Skill Checks tragen nur zum Log bei
    // Optional: Advantage/Disadvantage Systeme hier
    return true;
}

/*
-----------------------------------------------------------
 Saving Throws
-----------------------------------------------------------
*/

bool DndState::applySavingThrow(const DndEventTx& evt,
                                EncounterState& enc,
                                std::string& err)
{
    // Saving Throws beeinflussen ebenfalls nur das Log
    // (außer bei bestimmten Zaubern → später!)
    return true;
}


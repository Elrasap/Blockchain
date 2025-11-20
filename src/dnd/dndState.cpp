#include "dnd/dndState.hpp"
#include "dnd/stateSnapshot.hpp"
#include "dnd/dndPayload.hpp"
#include "core/blockchain.hpp"
#include <iostream>

using dnd::combat::CombatActorKind;

namespace dnd {

// ============================================================================
//  HP-Helper
// ============================================================================

void DndState::setMonsterHp(const std::string& id, int hp)
{
    if (hp < 0) hp = 0;

    auto& mon = monsters[id];
    if (mon.id.empty())
        mon.id = id;

    mon.hp = hp;
    if (mon.maxHp < hp)
        mon.maxHp = hp;
}

int DndState::getMonsterHp(const std::string& id) const
{
    auto it = monsters.find(id);
    if (it == monsters.end())
        return 0;
    return it->second.hp;
}

void DndState::setCharacterHp(const std::string& id, int hp)
{
    if (hp < 0) hp = 0;

    auto& cs = characters[id];
    cs.sheet.hpCurrent = hp;
}

int DndState::getCharacterHp(const std::string& id) const
{
    auto it = characters.find(id);
    if (it == characters.end())
        return 0;
    return it->second.sheet.hpCurrent;
}

// ============================================================================
//  apply() – verarbeitet jedes Event basierend auf actorType & note
// ============================================================================

bool DndState::apply(const DndEventTx& evt, std::string& err)
{
    // ======================================================
    // 0) Event typ prüfen
    // ======================================================
    switch(evt.eventType)
    {
        case DndEventType::CreateCharacter:
            break;
        case DndEventType::SpawnMonster:
            break;
        case DndEventType::StartEncounter:
            break;
        case DndEventType::Initiative:
            break;
        case DndEventType::Hit:
            break;
        case DndEventType::Damage:
            break;
        case DndEventType::SkillCheck:
            break;
        case DndEventType::EndEncounter:
            break;
        default:
            err = "Unknown eventType";
            return false;
    }

    // ======================================================
    // 1) Encounter automatisch erstellen
    // ======================================================
    auto encIt = encounters.find(evt.encounterId);
    if (encIt == encounters.end()) {
        encounters[evt.encounterId] = EncounterState{
            evt.encounterId,
            true,
            1,
            0,
            {},
            {}
        };
        encIt = encounters.find(evt.encounterId);
    }

    EncounterState& enc = encIt->second;

    // Event ins Log aufnehmen
    enc.events.push_back(evt);

    // ======================================================
    // EventType Dispatch
    // ======================================================

    switch(evt.eventType)
    {
        // --------------------------------------------------
        // CHARACTER CREATION
        // --------------------------------------------------
        case DndEventType::CreateCharacter:
        {
            auto& cs = characters[evt.actorId];

            // default HP
            if (cs.sheet.hpCurrent == 0)
                cs.sheet.hpCurrent = 10;

            if (cs.ownerPubKey.empty() && !evt.senderPubKey.empty())
                cs.ownerPubKey = evt.senderPubKey;

            return true;
        }

        // --------------------------------------------------
        // SPAWN MONSTER
        // --------------------------------------------------
        case DndEventType::SpawnMonster:
        {
            auto& mon = monsters[evt.actorId];
            mon.id = evt.actorId;

            if (mon.maxHp == 0)
                mon.maxHp = 10;

            mon.hp = mon.maxHp;
            return true;
        }

        // --------------------------------------------------
        // START ENCOUNTER
        // --------------------------------------------------
        case DndEventType::StartEncounter:
        {
            enc.active = true;
            enc.round = 1;
            enc.turnIndex = 0;
            return true;
        }

        // --------------------------------------------------
        // INITIATIVE (nur Logging)
        // --------------------------------------------------
        case DndEventType::Initiative:
            return true;

        // --------------------------------------------------
        // HIT (nur Flag, keine HP Änderung)
        // --------------------------------------------------
        case DndEventType::Hit:
            return true;

        // --------------------------------------------------
        // DAMAGE (HP ändern)
        // --------------------------------------------------
        case DndEventType::Damage:
        {
            if (evt.damage <= 0)
                return true;

            // character
            if (evt.targetType == 0)
            {
                auto& cs = characters[evt.targetId];
                if (cs.sheet.hpCurrent == 0)
                    cs.sheet.hpCurrent = 10;

                cs.sheet.hpCurrent -= evt.damage;
                if (cs.sheet.hpCurrent < 0)
                    cs.sheet.hpCurrent = 0;

                return true;
            }

            // monster
            if (evt.targetType == 1)
            {
                auto& mon = monsters[evt.targetId];
                if (mon.maxHp == 0)
                    mon.maxHp = 10;
                if (mon.hp == 0)
                    mon.hp = mon.maxHp;

                mon.hp -= evt.damage;
                if (mon.hp < 0)
                    mon.hp = 0;

                if (mon.hp == 0)
                {
                    // wenn alle Monster tot → Encounter enden
                    bool allDead = true;

                    for (const auto& ev : enc.events)
                    {
                        if (ev.targetType == 1)
                        {
                            auto it = monsters.find(ev.targetId);
                            if (it != monsters.end() && it->second.hp > 0)
                            {
                                allDead = false;
                                break;
                            }
                        }
                    }

                    if (allDead)
                        enc.active = false;
                }

                return true;
            }

            err = "Invalid targetType";
            return false;
        }

        // --------------------------------------------------
        // SKILL CHECK (nur Logging)
        // --------------------------------------------------
        case DndEventType::SkillCheck:
            return true;

        // --------------------------------------------------
        // END ENCOUNTER
        // --------------------------------------------------
        case DndEventType::EndEncounter:
        {
            enc.active = false;
            return true;
        }
    }

    return true;
}

// ============================================================================
// Snapshots
// ============================================================================

bool DndState::saveSnapshot(const std::string& path, std::string& err) const
{
    if (!writeSnapshot(*this, path)) {
        err = "Failed to write snapshot";
        return false;
    }
    return true;
}

bool DndState::loadSnapshot(const std::string& path, std::string& err)
{
    if (!dnd::loadSnapshot(*this, path)) {
        err = "Failed to load snapshot";
        return false;
    }
    return true;
}

// ============================================================================
// State aus Blockchain rekonstruieren
// ============================================================================

bool DndState::rebuildFromChain(const ::Blockchain& chain, std::string& err)
{
    clear();

    for (const auto& block : chain.getChain())
    {
        for (const auto& tx : block.transactions)
        {
            if (!dnd::isDndPayload(tx.payload))
                continue;

            auto evt = dnd::decodeDndTx(tx.payload);
            evt.senderPubKey = tx.senderPubkey;
            evt.signature    = tx.signature;

            if (!apply(evt, err))
                return false;
        }
    }

    return true;
}

} // namespace dnd


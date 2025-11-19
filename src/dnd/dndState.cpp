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
    // 1) Encounter holen oder ggf. neu anlegen
    //    (auch wenn encounterId leer ist, ist ein Eintrag
    //     mit "" als Key nicht schlimm)
    // ======================================================
    auto encIt = encounters.find(evt.encounterId);
    if (encIt == encounters.end()) {
        EncounterState enc;
        enc.id        = evt.encounterId;
        enc.active    = true;
        enc.round     = 1;
        enc.turnIndex = 0;

        encIt = encounters.emplace(enc.id, enc).first;
    }

    EncounterState& enc = encIt->second;

    // Event ins Log schreiben
    enc.events.push_back(evt);

    // ======================================================
    // 2) Character-Ownership setzen (falls actor ein Character ist)
    //    Idee: Der erste Event mit actorId X "claimt" den Charakter.
    // ======================================================
    if (evt.actorType == 0 && !evt.actorId.empty()) {
        auto& cs = characters[evt.actorId]; // auto-create

        // Falls HP noch nie gesetzt → Default-HP
        if (cs.sheet.hpCurrent == 0) {
            cs.sheet.hpCurrent = 10;  // einfache Default-Logik
        }

        // Owner einmalig setzen, wenn noch leer
        if (cs.ownerPubKey.empty() && !evt.senderPubKey.empty()) {
            cs.ownerPubKey = evt.senderPubKey;
        }
    }

    // ======================================================
    // 3) Kampflogik: nur wenn Hit + Damage
    // ======================================================
    if (evt.hit && evt.damage > 0)
    {
        // --------------------------------------------------
        // DAMAGE ON CHARACTER (targetType == 0)
        // --------------------------------------------------
        if (evt.targetType == 0) {
            auto& cs = characters[evt.targetId]; // auto-create

            // Default-HP, falls der Char vorher nicht bekannt war
            if (cs.sheet.hpCurrent == 0) {
                cs.sheet.hpCurrent = 10;
            }

            cs.sheet.hpCurrent -= evt.damage;
            if (cs.sheet.hpCurrent < 0)
                cs.sheet.hpCurrent = 0;
        }
        // --------------------------------------------------
        // DAMAGE ON MONSTER (targetType == 1)
        // --------------------------------------------------
        else if (evt.targetType == 1) {
            auto& mon = monsters[evt.targetId]; // auto-create

            if (mon.id.empty())
                mon.id = evt.targetId;

            // Wenn Monster neu ist: Default-HP
            if (mon.maxHp == 0 && mon.hp == 0) {
                mon.maxHp = 10;
                mon.hp    = mon.maxHp;
            }

            mon.hp -= evt.damage;
            if (mon.hp < 0)
                mon.hp = 0;

            // Wenn dieses Monster tot ist → prüfen,
            // ob ALLE Monster im Encounter tot sind.
            if (mon.hp == 0) {
                bool allDead = true;

                for (const auto& actor : enc.actors) {
                    if (actor.kind == CombatActorKind::Monster) {
                        auto itMon = monsters.find(actor.id);
                        if (itMon != monsters.end() && itMon->second.hp > 0) {
                            allDead = false;
                            break;
                        }
                    }
                }

                if (allDead) {
                    enc.active = false;
                }
            }
        }
        // --------------------------------------------------
        // Ungültiger targetType
        // --------------------------------------------------
        else {
            err = "Invalid targetType " + std::to_string(evt.targetType);
            return false;
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


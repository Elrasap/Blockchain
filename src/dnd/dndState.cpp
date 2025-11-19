#include "dnd/dndState.hpp"
#include "dnd/stateSnapshot.hpp"
#include "dnd/dndPayload.hpp"
#include "core/blockchain.hpp"
#include "core/block.hpp"
#include "core/transaction.hpp"
#include <iostream>

using dnd::combat::CombatActorKind;

namespace dnd {

// ========================================================
// HP-Helper für Tests / Game-Logik
// ========================================================

void DndState::setMonsterHp(const std::string& id, int hp)
{
    if (hp < 0) hp = 0;

    auto& mon = monsters[id];   // legt ggf. neuen MonsterState an
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

    auto& cs = characters[id];  // legt ggf. neuen CharacterState+Sheet an
    cs.sheet.hpCurrent = hp;
}

int DndState::getCharacterHp(const std::string& id) const
{
    auto it = characters.find(id);
    if (it == characters.end())
        return 0;
    return it->second.sheet.hpCurrent;
}

// ========================================================
// apply() – ein DnD-Event auf den State anwenden
// ========================================================
bool DndState::apply(const DndEventTx& evt, std::string& err)
{
    switch (evt.type)
    {
    // ======================================================
    //  CREATE_CHARACTER
    // ======================================================
    case DndEventType::CREATE_CHARACTER: {
        Character c;
        c.id          = evt.actorId;
        c.name        = evt.note;
        c.hp          = 10;
        c.maxHp       = 10;
        c.level       = 1;

        // NEU: Ownership
        c.ownerPubKey = evt.senderPubKey;

        characters[c.id] = c;
        return true;
    }

    // ======================================================
    //  SPAWN_MONSTER
    // ======================================================
    case DndEventType::SPAWN_MONSTER: {
        Monster m;
        m.id    = evt.actorId;
        m.hp    = 10;
        m.maxHp = 10;

        monsters[m.id] = m;
        return true;
    }

    default:
        break;
    }

    // ======================================================
    //  Für alle Kampf-Events: Encounter holen oder erstellen
    // ======================================================
    auto encIt = encounters.find(evt.encounterId);
    if (encIt == encounters.end()) {
        EncounterState enc;
        enc.id        = evt.encounterId;
        enc.active    = true;
        enc.round     = 1;
        enc.turnIndex = 0;
        encounters.emplace(enc.id, enc);
        encIt = encounters.find(enc.id);
    }

    EncounterState& enc = encIt->second;

    // Log speichern
    enc.events.push_back(evt);

    // ======================================================
    //  HIT / DAMAGE
    // ======================================================
    if (evt.hit && evt.damage > 0)
    {
        // DAMAGE ON CHARACTER
        if (evt.targetType == 0)
        {
            auto it = characters.find(evt.targetId);
            if (it == characters.end()) {
                err = "Unknown character " + evt.targetId;
                return false;
            }

            Character& c = it->second;
            c.hp -= evt.damage;
            if (c.hp < 0) c.hp = 0;
        }
        // DAMAGE ON MONSTER
        else if (evt.targetType == 1)
        {
            auto& m = monsters[evt.targetId];

            if (m.id.empty())
                m.id = evt.targetId;

            if (m.maxHp == 0)
                m.maxHp = m.hp = 10;

            m.hp -= evt.damage;
            if (m.hp < 0) m.hp = 0;

            // Auto-End Encounter wenn alle Monster tot
            if (m.hp == 0)
            {
                bool allDead = true;

                for (auto& actor : enc.actors)
                {
                    if (actor.kind == CombatActorKind::Monster)
                    {
                        auto it2 = monsters.find(actor.id);
                        if (it2 != monsters.end() && it2->second.hp > 0) {
                            allDead = false;
                            break;
                        }
                    }
                }

                if (allDead)
                    enc.active = false;
            }
        }
        else
        {
            err = "Invalid targetType " + std::to_string(evt.targetType);
            return false;
        }
    }

    return true;
}

// ========================================================
// Snapshot speichern
// ========================================================
bool DndState::saveSnapshot(const std::string& path, std::string& err) const
{
    if (!dnd::writeSnapshot(*this, path)) {
        err = "Failed to write snapshot to: " + path;
        return false;
    }
    return true;
}

// ========================================================
// Snapshot laden
// ========================================================
bool DndState::loadSnapshot(const std::string& path, std::string& err)
{
    if (!dnd::loadSnapshot(*this, path)) {
        err = "Failed to load snapshot from: " + path;
        return false;
    }
    return true;
}

// ========================================================
// State aus der Blockchain neu aufbauen
// ========================================================
bool DndState::rebuildFromChain(const ::Blockchain& chain, std::string& err)
{
    clear();

    const auto& blocks = chain.getChain();

    std::cout << "[DndState] Rebuilding from " << blocks.size() << " blocks...\n";

    for (const auto& block : blocks)
    {
        for (const auto& tx : block.transactions)
        {
            // Nur DnD-Payloads berücksichtigen
            if (!dnd::isDndPayload(tx.payload))
                continue;

            // Payload → DndEventTx
            auto evt = dnd::decodeDndTx(tx.payload);
            evt.senderPubKey = tx.senderPubkey;
            evt.signature    = tx.signature;

            if (!apply(evt, err)) {
                std::cerr << "[DndState] apply() failed at block "
                          << block.header.height << ": " << err << "\n";
                return false;
            }
        }
    }

    std::cout << "[DndState] Rebuild complete. Characters: "
              << characters.size()
              << " Monsters: "  << monsters.size()
              << " Encounters: "<< encounters.size()
              << "\n";

    return true;
}

} // namespace dnd


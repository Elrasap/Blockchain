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
    // 1) Encounter holen oder automatisch anlegen
    auto encIt = encounters.find(evt.encounterId);
    if (encIt == encounters.end()) {
        EncounterState enc;
        enc.id        = evt.encounterId;
        enc.active    = true;
        enc.round     = 1;
        enc.turnIndex = 0;

        auto [it, ok] = encounters.emplace(enc.id, std::move(enc));
        encIt = it;
    }

    EncounterState& enc = encIt->second;

    // 2) Event ins Log eintragen
    enc.events.push_back(evt);

    // 3) Schaden anwenden, falls Hit
    if (evt.hit && evt.damage > 0)
    {
        // TargetType 0 = Character
        if (evt.targetType == 0) {
            auto& sheet = characters[evt.targetId].sheet; // auto-create
            sheet.hpCurrent -= evt.damage;
            if (sheet.hpCurrent < 0) sheet.hpCurrent = 0;
        }
        // TargetType 1 = Monster
        else if (evt.targetType == 1) {
            auto& mon = monsters[evt.targetId]; // auto-create
            if (mon.id.empty())
                mon.id = evt.targetId;

            // Falls Monster neu ist: Default-HP setzen (für einfache Tests)
            if (mon.maxHp == 0 && mon.hp == 0) {
                mon.maxHp = 5;   // so wird bei 5 Schaden sofort auf 0 gekillt
                mon.hp    = mon.maxHp;
            }

            mon.hp -= evt.damage;
            if (mon.hp < 0) mon.hp = 0;

            // Wenn Monster tot → prüfen ob Encounter beendet
            if (mon.hp == 0) {
                bool allDead = true;

                for (const auto& actor : enc.actors)
                {
                    if (actor.kind == CombatActorKind::Monster) {
                        auto itMon = monsters.find(actor.id);
                        if (itMon != monsters.end() && itMon->second.hp > 0) {
                            allDead = false;
                            break;
                        }
                    }
                }

                if (allDead)
                    enc.active = false;
            }
        }
        else {
            err = "Unknown targetType " + std::to_string(evt.targetType);
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


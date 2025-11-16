#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "dnd/character.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/dndTx.hpp"

namespace dnd {

// =====================================================
// CharacterState
// =====================================================
struct CharacterState {
    CharacterSheet sheet;
};

// =====================================================
// MonsterState
// =====================================================
struct MonsterState {
    std::string id;
    int hp = 0;
    int maxHp = 0;
};

// =====================================================
// EncounterState
// =====================================================
struct EncounterState {
    std::string id;
    bool active = true;
    int round = 1;
    int turnIndex = 0;

    std::vector<combat::CombatActorRef> actors;
    std::vector<DndEventTx> events;     // Log historisch
};

// =====================================================
// DndState – Zentrale Spielwelt
// =====================================================
class DndState {
public:
    std::unordered_map<std::string, CharacterState> characters;
    std::unordered_map<std::string, MonsterState>   monsters;
    std::unordered_map<std::string, EncounterState> encounters;

    // --------------------------------------------
    // Reset
    // --------------------------------------------
    void clear() {
        characters.clear();
        monsters.clear();
        encounters.clear();
    }

    // --------------------------------------------
    // Existence checks
    // --------------------------------------------
    bool characterExists(const std::string& id) const {
        return characters.count(id) > 0;
    }

    bool monsterExists(const std::string& id) const {
        return monsters.count(id) > 0;
    }

    bool encounterExists(const std::string& id) const {
        return encounters.count(id) > 0;
    }

    // --------------------------------------------
    // Validierung (für TX)
    // --------------------------------------------
    bool validate(const DndEventTx& evt, std::string& err) const {
        if (!encounterExists(evt.encounterId)) {
            err = "Encounter does not exist";
            return false;
        }
        if (!characterExists(evt.actorId) && !monsterExists(evt.actorId)) {
            err = "Actor does not exist";
            return false;
        }
        if (!characterExists(evt.targetId) && !monsterExists(evt.targetId)) {
            err = "Target does not exist";
            return false;
        }
        return true;
    }

    // --------------------------------------------
    // Apply (TX → State ändern)
    // --------------------------------------------
    bool apply(const DndEventTx& evt, std::string& err) {
        // Validate
        if (!validate(evt, err)) return false;

        auto& enc = encounters[evt.encounterId];
        enc.events.push_back(evt);

        // Damage apply
        if (evt.hit && evt.damage > 0) {
            if (characterExists(evt.targetId)) {
                auto& c = characters[evt.targetId].sheet;
                c.hpCurrent -= evt.damage;
                if (c.hpCurrent < 0) c.hpCurrent = 0;
            } else if (monsterExists(evt.targetId)) {
                auto& m = monsters[evt.targetId];
                m.hp -= evt.damage;
                if (m.hp < 0) m.hp = 0;

                if (m.hp == 0) {
                    // Encounter finished
                    enc.active = false;
                }
            }
        }

        return true;
    }

};

} // namespace dnd


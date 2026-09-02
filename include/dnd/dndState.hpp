#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

#include "dnd/character.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/dndTx.hpp"


namespace dnd {




struct CharacterState {
    CharacterSheet sheet;
    std::vector<uint8_t> ownerPubKey;
};




struct MonsterState {
    std::string id;
    int hp     = 0;
    int maxHp  = 0;
};




struct EncounterState {
    std::string id;
    bool active = true;
    int round   = 1;
    int turnIndex = 0;

    std::vector<combat::CombatActorRef> actors;
    std::vector<DndEventTx>             events;
};

class DndState {
public:
    std::unordered_map<std::string, CharacterState> characters;
    std::unordered_map<std::string, MonsterState>   monsters;
    std::unordered_map<std::string, EncounterState> encounters;

    bool apply(const DndEventTx& evt, std::string& err);


    int  getMonsterHp(const std::string& id) const;

    int  getCharacterHp(const std::string& id) const;


    void clear() {
        characters.clear();
        monsters.clear();
        encounters.clear();
    }


    bool characterExists(const std::string& id) const {
        return characters.find(id) != characters.end();
    }

    bool monsterExists(const std::string& id) const {
        return monsters.find(id) != monsters.end();
    }

    bool encounterExists(const std::string& id) const {
        return encounters.find(id) != encounters.end();
    }

    bool encounterActive(const std::string& id) const {
        auto it = encounters.find(id);
        return it != encounters.end() && it->second.active;
    }

    const std::vector<uint8_t>* characterOwner(const std::string& id) const {
        auto it = characters.find(id);
        return it == characters.end() ? nullptr : &it->second.ownerPubKey;
    }

    EncounterState* getEncounter(const std::string& id) {
        auto it = encounters.find(id);
        return (it == encounters.end() ? nullptr : &it->second);
    }

    const EncounterState* getEncounter(const std::string& id) const {
        auto it = encounters.find(id);
        return (it == encounters.end() ? nullptr : &it->second);
    }
};

}

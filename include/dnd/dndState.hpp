#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

#include "dnd/character.hpp"
#include "dnd/combat/encounter.hpp"
#include "dnd/dndTx.hpp"


class Blockchain;

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




struct Character {
    std::string id;
    std::string name;
    int hp = 10;
    int maxHp = 10;
    int level = 1;

    std::vector<uint8_t> ownerPubKey;
};




class DndState {
public:
    std::unordered_map<std::string, CharacterState> characters;
    std::unordered_map<std::string, MonsterState>   monsters;
    std::unordered_map<std::string, EncounterState> encounters;

    bool apply(const DndEventTx& evt, std::string& err);


    void setMonsterHp(const std::string& id, int hp);
    int  getMonsterHp(const std::string& id) const;

    void setCharacterHp(const std::string& id, int hp);
    int  getCharacterHp(const std::string& id) const;


    void clear() {
        characters.clear();
        monsters.clear();
        encounters.clear();
    }


    bool saveSnapshot(const std::string& path, std::string& err) const;
    bool loadSnapshot(const std::string& path, std::string& err);


    bool rebuildFromChain(const ::Blockchain& chain, std::string& err);


    bool characterExists(const std::string& id) const {
        return characters.find(id) != characters.end();
    }

    bool monsterExists(const std::string& id) const {
        return monsters.find(id) != monsters.end();
    }

    bool encounterExists(const std::string& id) const {
        return encounters.find(id) != encounters.end();
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


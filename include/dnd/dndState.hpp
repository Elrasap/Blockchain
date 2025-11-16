#pragma once
#include <unordered_map>
#include <string>
#include "dnd/dndTx.hpp"
#include "dnd/character.hpp"
#include "dnd/monster.hpp"
#include "dnd/encounter.hpp"

namespace dnd {

class DndState {
public:
    // Die State-Maps
    std::unordered_map<std::string, CharacterState> characters;
    std::unordered_map<std::string, MonsterState> monsters;
    std::unordered_map<std::string, EncounterState> encounters;

    // TX anwenden
    bool apply(const DndEventTx& evt, std::string& err);

    // CHAIN REPLAY!!!
    bool rebuildFromChain(const Blockchain& chain, std::string& err);
    bool saveSnapshot(const std::string& path, std::string& err) const;
    bool loadSnapshot(const std::string& path, std::string& err);


private:
    bool applyAttack(const DndEventTx& evt, EncounterState& enc, std::string& err);
    bool applySkillCheck(const DndEventTx& evt, EncounterState& enc, std::string& err);
    bool applySavingThrow(const DndEventTx& evt, EncounterState& enc, std::string& err);
};

} // namespace dnd


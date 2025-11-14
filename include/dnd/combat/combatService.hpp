#pragma once
#include <nlohmann/json.hpp>
#include "dnd/character.hpp"
#include "dnd/combat/payloads.hpp"
#include "dnd/combat/dice.hpp"

namespace dnd::combat {

class CombatService {
public:
    Dice& getDice() { return dice; }
    const Dice& getDice() const { return dice; }

    CombatService();

    void reseed(uint64_t seed);

    // Greift Charakter-HP an, passt defender.hpCurrent an
    nlohmann::json performAttack(const AttackPayload& payload,
                                 dnd::CharacterSheet& attacker,
                                 dnd::CharacterSheet& defender);

    // Keine HP-Änderung – nur Wurf/Erfolg
    nlohmann::json performSkillCheck(const SkillCheckPayload& payload,
                                     const dnd::CharacterSheet& actor);

    // Saving Throw, optional Schaden (full/half), passt HP an
    nlohmann::json performSavingThrow(const SavingThrowPayload& payload,
                                      dnd::CharacterSheet& actor);

    // Initiative-Wurf
    nlohmann::json rollInitiative(const InitiativePayload& payload,
                                  const dnd::CharacterSheet& actor);

private:
    Dice dice;

    int clampHp(int hp, int hpMax);
};

} // namespace dnd::combat


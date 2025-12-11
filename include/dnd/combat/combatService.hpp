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

    nlohmann::json performAttack(const AttackPayload& payload,
                                 dnd::CharacterSheet& attacker,
                                 dnd::CharacterSheet& defender);

    nlohmann::json performSkillCheck(const SkillCheckPayload& payload,
                                     const dnd::CharacterSheet& actor);

    nlohmann::json performSavingThrow(const SavingThrowPayload& payload,
                                      dnd::CharacterSheet& actor);

    nlohmann::json rollInitiative(const InitiativePayload& payload,
                                  const dnd::CharacterSheet& actor);

private:
    Dice dice;

    int clampHp(int hp, int hpMax);
};

}


#include "dnd/combat/combatService.hpp"

namespace dnd::combat {

CombatService::CombatService()
: dice() {}

void CombatService::reseed(uint64_t seed) {
    dice.reseed(seed);
}

int CombatService::clampHp(int hp, int hpMax) {
    if (hp < 0) return 0;
    if (hp > hpMax) return hpMax;
    return hp;
}

nlohmann::json CombatService::performAttack(const AttackPayload& payload,
                                            dnd::CharacterSheet& attacker,
                                            dnd::CharacterSheet& defender)
{
    // 1) Attack Roll
    D20Roll atkRoll = dice.rollD20(payload.attackBonus, payload.advantage);

    // Simple: Hit, wenn total >= (z.B. defender.armorClass, falls du das hast)
    // Sonst DC fix 10 oder 12 als Beispiel
    int ac = 10;
    if constexpr (requires(const dnd::CharacterSheet& c) { c.armorClass; }) {
        // Falls dein CharacterSheet armorClass hat – C++20 requires wäre fancy,
        // aber du kannst hier einfach ac = defender.armorClass; setzen,
        // wenn du dieses Feld hast.
    }

    bool isCrit = (atkRoll.natural == 20);
    bool isFumble = (atkRoll.natural == 1);
    bool hit;

    if (isCrit) {
        hit = true;
    } else if (isFumble) {
        hit = false;
    } else {
        hit = (atkRoll.total >= ac);
    }

    // 2) Schaden
    int dmgTotal = 0;
    DiceRoll dmgRoll;

    if (hit) {
        dmgRoll = dice.rollExpr(payload.damageExpr);
        dmgTotal = dmgRoll.total;
        if (isCrit) {
            // Einfacher Crit: Schaden verdoppeln
            dmgTotal *= 2;
        }

        int hpBefore = defender.hpCurrent;
        defender.hpCurrent = clampHp(defender.hpCurrent - dmgTotal, defender.hpMax);
        int hpAfter = defender.hpCurrent;

        nlohmann::json j;
        j["type"] = "attack";
        j["attackerId"] = payload.attackerId;
        j["targetId"] = payload.targetId;
        j["weaponName"] = payload.weaponName;
        j["attackRoll"] = atkRoll;
        j["hit"] = true;
        j["critical"] = isCrit;
        j["fumble"] = isFumble;
        j["damageRoll"] = dmgRoll;
        j["damageTotal"] = dmgTotal;
        j["hpBefore"] = hpBefore;
        j["hpAfter"] = hpAfter;
        return j;
    } else {
        nlohmann::json j;
        j["type"] = "attack";
        j["attackerId"] = payload.attackerId;
        j["targetId"] = payload.targetId;
        j["weaponName"] = payload.weaponName;
        j["attackRoll"] = atkRoll;
        j["hit"] = false;
        j["critical"] = isCrit;
        j["fumble"] = isFumble;
        j["damageTotal"] = 0;
        return j;
    }
}

nlohmann::json CombatService::performSkillCheck(const SkillCheckPayload& payload,
                                                const dnd::CharacterSheet& actor)
{
    D20Roll roll = dice.rollD20(payload.skillBonus, payload.advantage);

    bool success = false;
    if (payload.dc > 0) {
        success = (roll.total >= payload.dc);
    }

    nlohmann::json j;
    j["type"] = "skillCheck";
    j["actorId"] = payload.actorId;
    j["skillName"] = payload.skillName;
    j["dc"] = payload.dc;
    j["roll"] = roll;
    if (payload.dc > 0) {
        j["success"] = success;
    }
    return j;
}

nlohmann::json CombatService::performSavingThrow(const SavingThrowPayload& payload,
                                                 dnd::CharacterSheet& actor)
{
    D20Roll roll = dice.rollD20(payload.saveBonus, payload.advantage);
    bool success = (roll.total >= payload.dc);

    int damageTotal = 0;
    DiceRoll dmgRoll;
    int hpBefore = actor.hpCurrent;
    int hpAfter = actor.hpCurrent;

    if (!payload.damageExpr.empty()) {
        dmgRoll = dice.rollExpr(payload.damageExpr);
        damageTotal = dmgRoll.total;

        if (success && payload.halfOnSuccess) {
            damageTotal = damageTotal / 2;
        }

        actor.hpCurrent = clampHp(actor.hpCurrent - damageTotal, actor.hpMax);
        hpAfter = actor.hpCurrent;
    }

    nlohmann::json j;
    j["type"] = "savingThrow";
    j["actorId"] = payload.actorId;
    j["saveName"] = payload.saveName;
    j["dc"] = payload.dc;
    j["roll"] = roll;
    j["success"] = success;
    j["halfOnSuccess"] = payload.halfOnSuccess;
    j["damageExpr"] = payload.damageExpr;
    j["damageTotal"] = damageTotal;
    j["hpBefore"] = hpBefore;
    j["hpAfter"] = hpAfter;
    j["damageRoll"] = dmgRoll;
    return j;
}

nlohmann::json CombatService::rollInitiative(const InitiativePayload& payload,
                                             const dnd::CharacterSheet& actor)
{
    D20Roll roll = dice.rollD20(payload.initBonus, AdvantageState::Normal);

    nlohmann::json j;
    j["type"] = "initiative";
    j["actorId"] = payload.actorId;
    j["roll"] = roll;
    j["initiative"] = roll.total;
    return j;
}

} // namespace dnd::combat


#include "dnd/combat/combat_payload.hpp"

namespace dnd {

void to_json(nlohmann::json& j, const AttackPayload& p) {
    j = {
        {"attackerId", p.attackerId},
        {"targetId", p.targetId},
        {"weapon", p.weapon},
        {"attackBonus", p.attackBonus},
        {"damageDiceCount", p.damageDiceCount},
        {"damageDiceSides", p.damageDiceSides}
    };
}
void from_json(const nlohmann::json& j, AttackPayload& p) {
    j.at("attackerId").get_to(p.attackerId);
    j.at("targetId").get_to(p.targetId);
    j.at("weapon").get_to(p.weapon);
    j.at("attackBonus").get_to(p.attackBonus);
    j.at("damageDiceCount").get_to(p.damageDiceCount);
    j.at("damageDiceSides").get_to(p.damageDiceSides);
}

void to_json(nlohmann::json& j, const SkillCheckPayload& p) {
    j = {{"characterId", p.characterId}, {"skill", p.skill}, {"bonus", p.bonus}};
}
void from_json(const nlohmann::json& j, SkillCheckPayload& p) {
    j.at("characterId").get_to(p.characterId);
    j.at("skill").get_to(p.skill);
    j.at("bonus").get_to(p.bonus);
}

void to_json(nlohmann::json& j, const SavingThrowPayload& p) {
    j = {
        {"characterId", p.characterId},
        {"type", p.type},
        {"bonus", p.bonus},
        {"dc", p.dc}
    };
}
void from_json(const nlohmann::json& j, SavingThrowPayload& p) {
    j.at("characterId").get_to(p.characterId);
    j.at("type").get_to(p.type);
    j.at("bonus").get_to(p.bonus);
    j.at("dc").get_to(p.dc);
}

void to_json(nlohmann::json& j, const InitiativePayload& p) {
    j = {{"characterId", p.characterId}, {"bonus", p.bonus}};
}
void from_json(const nlohmann::json& j, InitiativePayload& p) {
    j.at("characterId").get_to(p.characterId);
    j.at("bonus").get_to(p.bonus);
}

}


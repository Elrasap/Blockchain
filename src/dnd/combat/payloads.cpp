#include "dnd/combat/payloads.hpp"

namespace dnd::combat {

std::string advantageToString(AdvantageState s) {
    switch (s) {
        case AdvantageState::Advantage:     return "advantage";
        case AdvantageState::Disadvantage:  return "disadvantage";
        case AdvantageState::Normal:
        default:                            return "normal";
    }
}

AdvantageState advantageFromString(const std::string& s) {
    if (s == "advantage")    return AdvantageState::Advantage;
    if (s == "disadvantage") return AdvantageState::Disadvantage;
    return AdvantageState::Normal;
}

/* ------------ AttackPayload ------------ */
void to_json(nlohmann::json& j, const AttackPayload& p) {
    j = nlohmann::json{
        {"attackerId", p.attackerId},
        {"targetId",   p.targetId},
        {"weaponName", p.weaponName},
        {"attackBonus", p.attackBonus},
        {"damageExpr", p.damageExpr},
        {"advantage", advantageToString(p.advantage)}
    };
}

void from_json(const nlohmann::json& j, AttackPayload& p) {
    j.at("attackerId").get_to(p.attackerId);
    j.at("targetId").get_to(p.targetId);
    j.at("weaponName").get_to(p.weaponName);
    j.at("attackBonus").get_to(p.attackBonus);
    j.at("damageExpr").get_to(p.damageExpr);
    if (j.contains("advantage")) {
        p.advantage = advantageFromString(j.at("advantage").get<std::string>());
    } else {
        p.advantage = AdvantageState::Normal;
    }
}

/* ------------ SkillCheckPayload ------------ */
void to_json(nlohmann::json& j, const SkillCheckPayload& p) {
    j = nlohmann::json{
        {"actorId", p.actorId},
        {"skillName", p.skillName},
        {"skillBonus", p.skillBonus},
        {"dc", p.dc},
        {"advantage", advantageToString(p.advantage)}
    };
}

void from_json(const nlohmann::json& j, SkillCheckPayload& p) {
    j.at("actorId").get_to(p.actorId);
    j.at("skillName").get_to(p.skillName);
    j.at("skillBonus").get_to(p.skillBonus);
    if (j.contains("dc")) {
        p.dc = j.at("dc").get<int>();
    } else {
        p.dc = 0;
    }
    if (j.contains("advantage")) {
        p.advantage = advantageFromString(j.at("advantage").get<std::string>());
    } else {
        p.advantage = AdvantageState::Normal;
    }
}

/* ------------ SavingThrowPayload ------------ */
void to_json(nlohmann::json& j, const SavingThrowPayload& p) {
    j = nlohmann::json{
        {"actorId", p.actorId},
        {"saveName", p.saveName},
        {"saveBonus", p.saveBonus},
        {"dc", p.dc},
        {"halfOnSuccess", p.halfOnSuccess},
        {"damageExpr", p.damageExpr},
        {"advantage", advantageToString(p.advantage)}
    };
}

void from_json(const nlohmann::json& j, SavingThrowPayload& p) {
    j.at("actorId").get_to(p.actorId);
    j.at("saveName").get_to(p.saveName);
    j.at("saveBonus").get_to(p.saveBonus);
    j.at("dc").get_to(p.dc);
    j.at("halfOnSuccess").get_to(p.halfOnSuccess);
    j.at("damageExpr").get_to(p.damageExpr);
    if (j.contains("advantage")) {
        p.advantage = advantageFromString(j.at("advantage").get<std::string>());
    } else {
        p.advantage = AdvantageState::Normal;
    }
}

/* ------------ InitiativePayload ------------ */
void to_json(nlohmann::json& j, const InitiativePayload& p) {
    j = nlohmann::json{
        {"actorId", p.actorId},
        {"initBonus", p.initBonus}
    };
}

void from_json(const nlohmann::json& j, InitiativePayload& p) {
    j.at("actorId").get_to(p.actorId);
    if (j.contains("initBonus")) {
        j.at("initBonus").get_to(p.initBonus);
    } else {
        p.initBonus = 0;
    }
}

} // namespace dnd::combat


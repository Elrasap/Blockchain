#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace dnd::combat {

enum class AdvantageState {
    Normal,
    Advantage,
    Disadvantage
};

struct AttackPayload {
    std::string attackerId;
    std::string targetId;
    std::string weaponName;
    int attackBonus = 0;
    std::string damageExpr;
    AdvantageState advantage = AdvantageState::Normal;
};


struct SkillCheckPayload {
    std::string actorId;
    std::string skillName;
    int skillBonus = 0;
    AdvantageState advantage = AdvantageState::Normal;
    int dc = 0;
};


struct SavingThrowPayload {
    std::string actorId;
    std::string saveName;
    int saveBonus = 0;
    AdvantageState advantage = AdvantageState::Normal;
    int dc = 0;
    bool halfOnSuccess = false;
    std::string damageExpr;
};


struct InitiativePayload {
    std::string actorId;
    int initBonus = 0;
};


void to_json(nlohmann::json& j, const AttackPayload& p);
void from_json(const nlohmann::json& j, AttackPayload& p);

void to_json(nlohmann::json& j, const SkillCheckPayload& p);
void from_json(const nlohmann::json& j, SkillCheckPayload& p);

void to_json(nlohmann::json& j, const SavingThrowPayload& p);
void from_json(const nlohmann::json& j, SavingThrowPayload& p);

void to_json(nlohmann::json& j, const InitiativePayload& p);
void from_json(const nlohmann::json& j, InitiativePayload& p);


std::string advantageToString(AdvantageState s);
AdvantageState advantageFromString(const std::string& s);

}


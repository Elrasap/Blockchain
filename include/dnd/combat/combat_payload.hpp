#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace dnd {

struct AttackPayload {
    std::string attackerId;
    std::string targetId;
    std::string weapon;
    int attackBonus = 0;
    int damageDiceCount = 1;
    int damageDiceSides = 6;
};

struct SkillCheckPayload {
    std::string characterId;
    std::string skill;  // z.B. "Stealth"
    int bonus = 0;
};

struct SavingThrowPayload {
    std::string characterId;
    std::string type;   // "DEX", "WIS", ...
    int bonus = 0;
    int dc = 10;
};

struct InitiativePayload {
    std::string characterId;
    int bonus = 0;
};

// JSON Konverter:
void to_json(nlohmann::json& j, const AttackPayload& p);
void from_json(const nlohmann::json& j, AttackPayload& p);

void to_json(nlohmann::json& j, const SkillCheckPayload& p);
void from_json(const nlohmann::json& j, SkillCheckPayload& p);

void to_json(nlohmann::json& j, const SavingThrowPayload& p);
void from_json(const nlohmann::json& j, SavingThrowPayload& p);

void to_json(nlohmann::json& j, const InitiativePayload& p);
void from_json(const nlohmann::json& j, InitiativePayload& p);

}


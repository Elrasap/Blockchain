#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace dnd::combat {

enum class AdvantageState {
    Normal,
    Advantage,
    Disadvantage
};

// Basis-Angriff: 1 Angreifer, 1 Ziel, Attacke + Schaden
struct AttackPayload {
    std::string attackerId;
    std::string targetId;
    std::string weaponName;   // z.B. "Longsword"
    int attackBonus = 0;      // z.B. Proficiency + STR/DEX
    std::string damageExpr;   // z.B. "1d8+3"
    AdvantageState advantage = AdvantageState::Normal;
};

// Skill Check: z.B. Stealth, Athletics, Perception
struct SkillCheckPayload {
    std::string actorId;
    std::string skillName;    // z.B. "stealth"
    int skillBonus = 0;
    AdvantageState advantage = AdvantageState::Normal;
    int dc = 0;               // 0 = kein DC, nur Ergebnis anzeigen
};

// Saving Throw: z.B. DEX Save vs. Fireball
struct SavingThrowPayload {
    std::string actorId;
    std::string saveName;     // "dex", "con", ...
    int saveBonus = 0;
    AdvantageState advantage = AdvantageState::Normal;
    int dc = 0;
    bool halfOnSuccess = false;
    std::string damageExpr;   // Gesamtschaden bei Fail (halbiert bei Success, falls halfOnSuccess)
};

// Initiative-Wurf für einen Actor
struct InitiativePayload {
    std::string actorId;
    int initBonus = 0;
};

// JSON-Konvertierung
void to_json(nlohmann::json& j, const AttackPayload& p);
void from_json(const nlohmann::json& j, AttackPayload& p);

void to_json(nlohmann::json& j, const SkillCheckPayload& p);
void from_json(const nlohmann::json& j, SkillCheckPayload& p);

void to_json(nlohmann::json& j, const SavingThrowPayload& p);
void from_json(const nlohmann::json& j, SavingThrowPayload& p);

void to_json(nlohmann::json& j, const InitiativePayload& p);
void from_json(const nlohmann::json& j, InitiativePayload& p);

// AdvantageState <-> String
std::string advantageToString(AdvantageState s);
AdvantageState advantageFromString(const std::string& s);

} // namespace dnd::combat


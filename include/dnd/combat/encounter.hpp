#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace dnd::combat {

enum class CombatActorKind {
    Character = 0,
    Monster   = 1
};

struct CombatActorRef {
    std::string id;
    CombatActorKind kind   = CombatActorKind::Character;
    int initiative         = 0;
};

void to_json(nlohmann::json& j, const CombatActorRef& a);
void from_json(const nlohmann::json& j, CombatActorRef& a);

}

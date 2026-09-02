#include "dnd/combat/encounter.hpp"

namespace dnd::combat {

void to_json(nlohmann::json& json, const CombatActorRef& actor)
{
    json = {
        {"id", actor.id},
        {"kind", actor.kind == CombatActorKind::Character
            ? "character" : "monster"},
        {"initiative", actor.initiative}
    };
}

void from_json(const nlohmann::json& json, CombatActorRef& actor)
{
    actor.id = json.value("id", std::string{});
    actor.kind = json.value("kind", "character") == "monster"
        ? CombatActorKind::Monster : CombatActorKind::Character;
    actor.initiative = json.value("initiative", 0);
}

} // namespace dnd::combat

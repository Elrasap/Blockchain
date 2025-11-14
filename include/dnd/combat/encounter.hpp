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
    std::string id;        // characterId oder monsterId
    CombatActorKind kind   = CombatActorKind::Character;
    int initiative         = 0;
};

struct Encounter {
    std::string id;
    std::string name;
    bool active    = true;
    int round      = 1;
    int turnIndex  = 0;                 // Index in order
    std::vector<CombatActorRef> order;  // nach Initiative sortiert
};

class EncounterManager {
public:
    EncounterManager() = default;

    // Erzeugt ein neues Encounter, gibt Referenz zurück
    Encounter& startEncounter(const std::string& name);

    bool addCharacter(const std::string& encId,
                      const std::string& characterId,
                      int initiative);

    bool addMonster(const std::string& encId,
                    const std::string& monsterId,
                    int initiative);

    bool nextTurn(const std::string& encId);

    bool get(const std::string& encId, Encounter& out) const;
    std::vector<Encounter> list() const;

    // Hilfsfunktion für "aktives" Encounter (wir nehmen das letzte aktive)
    Encounter*       findActive();
    const Encounter* findActive() const;

private:
    std::vector<Encounter> encounters_;

    Encounter*       findMutable(const std::string& encId);
    const Encounter* findConst(const std::string& encId) const;
};

// JSON helpers
void to_json(nlohmann::json& j, const CombatActorRef& a);
void from_json(const nlohmann::json& j, CombatActorRef& a);
void to_json(nlohmann::json& j, const Encounter& e);
void from_json(const nlohmann::json& j, Encounter& e);

} // namespace dnd::combat


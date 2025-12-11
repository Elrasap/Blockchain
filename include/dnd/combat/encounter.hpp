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

struct Encounter {
    std::string id;
    std::string name;
    bool active    = true;
    int round      = 1;
    int turnIndex  = 0;
    std::vector<CombatActorRef> order;
};

class EncounterManager {
public:
    EncounterManager() = default;


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


    Encounter*       findActive();
    const Encounter* findActive() const;

private:
    std::vector<Encounter> encounters_;

    Encounter*       findMutable(const std::string& encId);
    const Encounter* findConst(const std::string& encId) const;
};


void to_json(nlohmann::json& j, const CombatActorRef& a);
void from_json(const nlohmann::json& j, CombatActorRef& a);
void to_json(nlohmann::json& j, const Encounter& e);
void from_json(const nlohmann::json& j, Encounter& e);

}


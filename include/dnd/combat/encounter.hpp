#pragma once
#include <string>
#include <vector>

namespace dnd {

enum class CombatActorKind {
    Character = 0,
    Monster   = 1
};

struct CombatActorRef {
    std::string id;        // characterId oder monsterId
    CombatActorKind kind;
    int initiative = 0;
};

struct Encounter {
    std::string id;
    std::string name;
    bool active = true;
    int round = 1;
    int turnIndex = 0;                 // Index in order
    std::vector<CombatActorRef> order; // nach Initiative sortiert
};

class EncounterService {
public:
    EncounterService() = default;

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

private:
    std::vector<Encounter> encounters_;

    Encounter* findMutable(const std::string& encId);
    const Encounter* findConst(const std::string& encId) const;
};

} // namespace dnd


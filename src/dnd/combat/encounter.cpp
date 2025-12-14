#include "dnd/combat/encounter.hpp"
#include <algorithm>
#include <random>

using nlohmann::json;

namespace dnd::combat {

static std::string genEncounterId() {
    static int counter = 1;
    return "enc-" + std::to_string(counter++);
}

// -------- JSON helper für CombatActorRef --------
void to_json(json& j, const CombatActorRef& a) {
    j = json{
        {"id", a.id},
        {"kind", a.kind == CombatActorKind::Character ? "character" : "monster"},
        {"initiative", a.initiative}
    };
}

void from_json(const json& j, CombatActorRef& a) {
    a.id = j.value("id", std::string{});
    std::string k = j.value("kind", "character");
    if (k == "monster")
        a.kind = CombatActorKind::Monster;
    else
        a.kind = CombatActorKind::Character;
    a.initiative = j.value("initiative", 0);
}

// -------- JSON helper für Encounter --------
void to_json(json& j, const Encounter& e) {
    j = json{
        {"id",        e.id},
        {"name",      e.name},
        {"active",    e.active},
        {"round",     e.round},
        {"turnIndex", e.turnIndex},
        {"order",     e.order}
    };
}

void from_json(const json& j, Encounter& e) {
    e.id        = j.value("id", std::string{});
    e.name      = j.value("name", std::string{});
    e.active    = j.value("active", true);
    e.round     = j.value("round", 1);
    e.turnIndex = j.value("turnIndex", 0);
    if (j.contains("order")) {
        e.order = j.at("order").get<std::vector<CombatActorRef>>();
    }
}

// -------- EncounterManager Implementierung --------

Encounter& EncounterManager::startEncounter(const std::string& name) {
    Encounter e;
    e.id   = genEncounterId();
    e.name = name;
    e.active = true;
    e.round = 1;
    e.turnIndex = 0;

    encounters_.push_back(e);
    return encounters_.back();
}

Encounter* EncounterManager::findMutable(const std::string& encId) {
    for (auto& e : encounters_) {
        if (e.id == encId) return &e;
    }
    return nullptr;
}

const Encounter* EncounterManager::findConst(const std::string& encId) const {
    for (auto& e : encounters_) {
        if (e.id == encId) return &e;
    }
    return nullptr;
}

Encounter* EncounterManager::findActive() {
    for (auto it = encounters_.rbegin(); it != encounters_.rend(); ++it) {
        if (it->active) return &*it;
    }
    return nullptr;
}

const Encounter* EncounterManager::findActive() const {
    for (auto it = encounters_.rbegin(); it != encounters_.rend(); ++it) {
        if (it->active) return &*it;
    }
    return nullptr;
}

bool EncounterManager::addCharacter(const std::string& encId,
                                    const std::string& characterId,
                                    int initiative) {
    Encounter* e = findMutable(encId);
    if (!e) return false;

    CombatActorRef a;
    a.id         = characterId;
    a.kind       = CombatActorKind::Character;
    a.initiative = initiative;

    e->order.push_back(a);

    std::sort(e->order.begin(), e->order.end(),
              [](const CombatActorRef& lhs, const CombatActorRef& rhs){
                  return lhs.initiative > rhs.initiative;
              });

    return true;
}

bool EncounterManager::addMonster(const std::string& encId,
                                  const std::string& monsterId,
                                  int initiative) {
    Encounter* e = findMutable(encId);
    if (!e) return false;

    CombatActorRef a;
    a.id         = monsterId;
    a.kind       = CombatActorKind::Monster;
    a.initiative = initiative;

    e->order.push_back(a);

    std::sort(e->order.begin(), e->order.end(),
              [](const CombatActorRef& lhs, const CombatActorRef& rhs){
                  return lhs.initiative > rhs.initiative;
              });

    return true;
}

bool EncounterManager::nextTurn(const std::string& encId) {
    Encounter* e = findMutable(encId);
    if (!e) return false;
    if (e->order.empty()) return false;

    e->turnIndex++;
    if (e->turnIndex >= static_cast<int>(e->order.size())) {
        e->turnIndex = 0;
        e->round++;
    }
    return true;
}

bool EncounterManager::get(const std::string& encId, Encounter& out) const {
    const Encounter* e = findConst(encId);
    if (!e) return false;
    out = *e;
    return true;
}

std::vector<Encounter> EncounterManager::list() const {
    return encounters_;
}

} // namespace dnd::combat


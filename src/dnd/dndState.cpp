#include "dnd/dndState.hpp"

#include <algorithm>

namespace dnd {

namespace {

combat::CombatActorKind actorKind(int type)
{
    return type == 1 ? combat::CombatActorKind::Monster
                     : combat::CombatActorKind::Character;
}

void rememberActor(EncounterState& encounter,
                   const std::string& id,
                   int type,
                   int initiative = 0)
{
    if (id.empty()) return;

    auto it = std::find_if(encounter.actors.begin(), encounter.actors.end(),
        [&](const combat::CombatActorRef& actor) {
            return actor.id == id && actor.kind == actorKind(type);
        });

    if (it == encounter.actors.end()) {
        encounter.actors.push_back({id, actorKind(type), initiative});
    } else if (initiative > 0) {
        it->initiative = initiative;
    }
}

} // namespace

int DndState::getMonsterHp(const std::string& id) const
{
    auto it = monsters.find(id);
    return it == monsters.end() ? 0 : it->second.hp;
}

int DndState::getCharacterHp(const std::string& id) const
{
    auto it = characters.find(id);
    return it == characters.end() ? 0 : it->second.sheet.hpCurrent;
}

bool DndState::apply(const DndEventTx& evt, std::string& err)
{
    err.clear();

    if (evt.actorType < 0 || evt.actorType > 1 ||
        evt.targetType < 0 || evt.targetType > 1) {
        err = "Invalid actor or target type";
        return false;
    }

    if (evt.eventType == DndEventType::CreateCharacter) {
        if (evt.actorId.empty() || characterExists(evt.actorId) ||
            (!evt.ownerPubKey.empty() && evt.ownerPubKey.size() != 32)) {
            err = evt.actorId.empty() ? "Character id required"
                : characterExists(evt.actorId) ? "Character already exists"
                                               : "Invalid character owner key";
            return false;
        }

        CharacterState character;
        character.sheet = makeDefaultCharacter(evt.actorId, evt.actorId,
                                               evt.actorId,
                                               CharacterClass::Custom,
                                               Race::Custom);
        character.ownerPubKey = evt.ownerPubKey.empty()
            ? evt.senderPubKey : evt.ownerPubKey;
        characters.emplace(evt.actorId, std::move(character));
        return true;
    }

    if (evt.eventType == DndEventType::SpawnMonster) {
        if (evt.actorId.empty() || monsterExists(evt.actorId)) {
            err = evt.actorId.empty() ? "Monster id required"
                                      : "Monster already exists";
            return false;
        }
        monsters.emplace(evt.actorId, MonsterState{evt.actorId, 10, 10});
        return true;
    }

    if (evt.eventType == DndEventType::StartEncounter) {
        if (evt.encounterId.empty() || encounterExists(evt.encounterId)) {
            err = evt.encounterId.empty() ? "Encounter id required"
                                          : "Encounter already exists";
            return false;
        }
        EncounterState encounter;
        encounter.id = evt.encounterId;
        encounter.active = true;
        encounter.round = 1;
        encounter.turnIndex = 0;
        encounter.events.push_back(evt);
        encounters.emplace(evt.encounterId, std::move(encounter));
        return true;
    }

    auto encounterIt = encounters.find(evt.encounterId);
    if (encounterIt == encounters.end()) {
        err = "Encounter not found";
        return false;
    }
    if (!encounterIt->second.active) {
        err = "Encounter is not active";
        return false;
    }

    EncounterState& encounter = encounterIt->second;

    if (!evt.actorId.empty()) {
        const bool actorExists = evt.actorType == 1
            ? monsterExists(evt.actorId) : characterExists(evt.actorId);
        if (!actorExists) {
            err = "Actor not found";
            return false;
        }
    }

    if (!evt.targetId.empty()) {
        const bool targetExists = evt.targetType == 1
            ? monsterExists(evt.targetId) : characterExists(evt.targetId);
        if (!targetExists) {
            err = "Target not found";
            return false;
        }
    }

    switch (evt.eventType) {
    case DndEventType::Initiative:
        if (evt.actorId.empty() || evt.roll < 1 || evt.roll > 20) {
            err = "Invalid initiative";
            return false;
        }
        rememberActor(encounter, evt.actorId, evt.actorType, evt.roll);
        std::stable_sort(encounter.actors.begin(), encounter.actors.end(),
            [](const combat::CombatActorRef& left,
               const combat::CombatActorRef& right) {
                return left.initiative > right.initiative;
            });
        break;

    case DndEventType::Hit:
        if (evt.actorId.empty() || evt.targetId.empty()) {
            err = "Hit requires actor and target";
            return false;
        }
        rememberActor(encounter, evt.actorId, evt.actorType);
        rememberActor(encounter, evt.targetId, evt.targetType);
        break;

    case DndEventType::Damage: {
        if (evt.targetId.empty() || evt.damage < 0) {
            err = "Invalid damage event";
            return false;
        }
        rememberActor(encounter, evt.actorId, evt.actorType);
        rememberActor(encounter, evt.targetId, evt.targetType);

        if (evt.targetType == 0) {
            auto& hp = characters.at(evt.targetId).sheet.hpCurrent;
            hp = std::max(0, hp - evt.damage);
        } else {
            auto& hp = monsters.at(evt.targetId).hp;
            hp = std::max(0, hp - evt.damage);

            bool hasMonster = false;
            bool allMonstersDead = true;
            for (const auto& actor : encounter.actors) {
                if (actor.kind != combat::CombatActorKind::Monster)
                    continue;
                hasMonster = true;
                auto it = monsters.find(actor.id);
                if (it != monsters.end() && it->second.hp > 0) {
                    allMonstersDead = false;
                    break;
                }
            }
            if (hasMonster && allMonstersDead)
                encounter.active = false;
        }
        break;
    }

    case DndEventType::SkillCheck:
        if (evt.actorId.empty() || evt.roll < 1 || evt.roll > 20) {
            err = "Invalid skill check";
            return false;
        }
        rememberActor(encounter, evt.actorId, evt.actorType);
        break;

    case DndEventType::EndEncounter:
        encounter.active = false;
        break;

    default:
        err = "Unknown eventType";
        return false;
    }

    encounter.events.push_back(evt);
    return true;
}

} // namespace dnd

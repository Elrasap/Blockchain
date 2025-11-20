#include "dnd/dndTxValidator.hpp"
#include <ctime>

namespace dnd {

DndTxValidator::DndTxValidator(DndValidationContext ctx)
    : ctx_(std::move(ctx)) {}

// ------------------- öffentliche API -------------------

bool DndTxValidator::validate(const DndEventTx& evt, std::string& error) const {
    // 0) Semantische Regeln je nach eventType
    if (!validateSemantic(evt, error))      return false;

    // 1) Existenz von Actor/Target
    if (!validateActorExists(evt, error))    return false;
    if (!validateTargetExists(evt, error))   return false;

    // 2) Encounter-Zustand
    if (!validateEncounter(evt, error))      return false;

    // 3) Berechtigungen
    if (!validatePermissions(evt, error))    return false;

    // 4) Zeitfenster
    if (!validateTimestamp(evt, error))      return false;

    return true;
}

// ------------------- 0) Semantische Regeln -------------------

bool DndTxValidator::validateSemantic(const DndEventTx& evt,
                                      std::string& error) const
{
    // Basis-Checks, die für fast alles gelten
    if (evt.encounterId.empty() &&
        evt.eventType != DndEventType::CreateCharacter &&
        evt.eventType != DndEventType::SpawnMonster) {
        error = "encounterId required for this eventType";
        return false;
    }

    switch (evt.eventType) {

    case DndEventType::CreateCharacter: {
        // Charakter erzeugen: actor = neuer Charakter
        if (evt.actorId.empty()) {
            error = "CreateCharacter requires actorId (character id)";
            return false;
        }
        if (evt.actorType != 0) {
            error = "CreateCharacter must have actorType=0 (character)";
            return false;
        }
        // Target, roll, damage, hit sind hier semantisch egal
        return true;
    }

    case DndEventType::SpawnMonster: {
        if (evt.actorId.empty()) {
            error = "SpawnMonster requires actorId (monster id)";
            return false;
        }
        if (evt.actorType != 1) {
            error = "SpawnMonster must have actorType=1 (monster)";
            return false;
        }
        return true;
    }

    case DndEventType::StartEncounter: {
        if (evt.encounterId.empty()) {
            error = "StartEncounter requires encounterId";
            return false;
        }
        // Optional: actorId/actorType können leer sein (DM-only Event)
        return true;
    }

    case DndEventType::Initiative: {
        if (evt.actorId.empty()) {
            error = "Initiative requires actorId";
            return false;
        }
        if (evt.roll < 1 || evt.roll > 20) {
            error = "Initiative roll must be between 1 and 20";
            return false;
        }
        return true;
    }

    case DndEventType::Hit: {
        if (evt.actorId.empty()) {
            error = "Hit requires actorId";
            return false;
        }
        if (evt.targetId.empty()) {
            error = "Hit requires targetId";
            return false;
        }
        if (evt.roll < 1 || evt.roll > 20) {
            error = "Hit roll must be between 1 and 20";
            return false;
        }
        // hit==true/false ist hier die DM-Entscheidung, wir erzwingen nur Plausibilität
        return true;
    }

    case DndEventType::Damage: {
        if (evt.targetId.empty()) {
            error = "Damage requires targetId";
            return false;
        }
        if (evt.damage < 0) {
            error = "Damage must be >= 0";
            return false;
        }
        // Wir erzwingen NICHT, dass vorher ein Hit-Event existiert.
        // Das wäre ein History-Check, den man optional später einbauen kann.
        return true;
    }

    case DndEventType::SkillCheck: {
        if (evt.actorId.empty()) {
            error = "SkillCheck requires actorId";
            return false;
        }
        if (evt.roll < 1 || evt.roll > 20) {
            error = "SkillCheck roll must be between 1 and 20";
            return false;
        }
        // Welche Skill (z.B. in note) bleibt dir überlassen.
        return true;
    }

    case DndEventType::EndEncounter: {
        if (evt.encounterId.empty()) {
            error = "EndEncounter requires encounterId";
            return false;
        }
        return true;
    }

    case DndEventType::Unknown:
    default:
        // Unknown-Events sind erlaubt, aber sehr restriktiv:
        if (evt.actorId.empty() && evt.targetId.empty()) {
            error = "Unknown eventType requires at least actorId or targetId";
            return false;
        }
        return true;
    }
}

// ------------------- 1) Actor-Existenz ---------------------

bool DndTxValidator::validateActorExists(const DndEventTx& evt,
                                         std::string& error) const {
    if (!ctx_.characterExists || !ctx_.monsterExists)
        return true;

    bool isSpawnLike =
        evt.eventType == DndEventType::CreateCharacter ||
        evt.eventType == DndEventType::SpawnMonster;

    // Spawn/Create erzeugt den Actor → keine Existenzprüfung
    if (isSpawnLike) {
        return true;
    }

    if (evt.actorId.empty())
        return true; // z.B. reines DM-Event

    bool isMonster = (evt.actorType == 1);
    if (isMonster) {
        if (!ctx_.monsterExists(evt.actorId)) {
            error = "Actor monster not found: " + evt.actorId;
            return false;
        }
    } else {
        if (!ctx_.characterExists(evt.actorId)) {
            error = "Actor character not found: " + evt.actorId;
            return false;
        }
    }
    return true;
}

// ------------------- 2) Target-Existenz ---------------------

bool DndTxValidator::validateTargetExists(const DndEventTx& evt,
                                          std::string& error) const {
    if (!ctx_.characterExists || !ctx_.monsterExists)
        return true;

    // Target optional oder nicht relevant bei bestimmten Events
    if (evt.targetId.empty())
        return true;

    bool isSpawnLike =
        evt.eventType == DndEventType::CreateCharacter ||
        evt.eventType == DndEventType::SpawnMonster ||
        evt.eventType == DndEventType::StartEncounter ||
        evt.eventType == DndEventType::Initiative;

    if (isSpawnLike) {
        return true;
    }

    bool isMonster = (evt.targetType == 1);
    if (isMonster) {
        if (!ctx_.monsterExists(evt.targetId)) {
            error = "Target monster not found: " + evt.targetId;
            return false;
        }
    } else {
        if (!ctx_.characterExists(evt.targetId)) {
            error = "Target character not found: " + evt.targetId;
            return false;
        }
    }
    return true;
}

// ------------------- 3) Encounter-Status ---------------------

bool DndTxValidator::validateEncounter(const DndEventTx& evt,
                                       std::string& error) const {
    if (!ctx_.encounterActive)
        return true;

    // Create/Spawn/StartEncounter dürfen auch "out of combat" passieren
    if (evt.eventType == DndEventType::CreateCharacter ||
        evt.eventType == DndEventType::SpawnMonster  ||
        evt.eventType == DndEventType::StartEncounter) {
        return true;
    }

    if (evt.encounterId.empty()) {
        error = "EncounterId required";
        return false;
    }

    if (!ctx_.encounterActive(evt.encounterId)) {
        error = "Encounter not active: " + evt.encounterId;
        return false;
    }
    return true;
}


// ------------------- 4) Berechtigungen ---------------------

bool DndTxValidator::validatePermissions(const DndEventTx& evt,
                                         std::string& error) const
{
    bool isMonster = (evt.actorType == 1);

    if (!ctx_.hasControlPermission)
        return true;

    if (evt.actorId.empty())
        return true; // z.B. DM-only Events wie StartEncounter

    if (!ctx_.hasControlPermission(evt.actorId, evt.senderPubKey, isMonster)) {
        error = "permission denied for actorId=" + evt.actorId;
        return false;
    }
    return true;
}

// ------------------- 5) Zeitfenster ---------------------

bool DndTxValidator::validateTimestamp(const DndEventTx& evt,
                                       std::string& error) const {
    uint64_t now = ctx_.nowOverride ? ctx_.nowOverride :
                                      static_cast<uint64_t>(std::time(nullptr));

    if (evt.timestamp > now + ctx_.maxFutureSkewSec) {
        error = "Event timestamp too far in the future";
        return false;
    }

    if (evt.timestamp + ctx_.maxPastAgeSec < now) {
        error = "Event timestamp too old";
        return false;
    }
    return true;
}

} // namespace dnd


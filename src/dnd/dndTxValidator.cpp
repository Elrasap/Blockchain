#include "dnd/dndTxValidator.hpp"
#include <ctime>

namespace dnd {

DndTxValidator::DndTxValidator(DndValidationContext ctx)
    : ctx_(std::move(ctx)) {}

// ------------------- öffentliche API -------------------

bool DndTxValidator::validate(const DndEventTx& evt, std::string& error) const {
    if (!validateActorExists(evt, error))    return false;
    if (!validateTargetExists(evt, error))   return false;
    if (!validateEncounter(evt, error))      return false;
    if (!validatePermissions(evt, error))    return false;
    if (!validateTimestamp(evt, error))      return false;
    return true;
}

// ------------------- private Helfer ---------------------

bool DndTxValidator::validateActorExists(const DndEventTx& evt,
                                         std::string& error) const {
    if (!ctx_.characterExists || !ctx_.monsterExists) {
        return true; // kein Kontext => keine Prüfung
    }

    const bool isMonster = (evt.actorType == 1);
    if (isMonster) {
        if (!ctx_.monsterExists(evt.actorId)) {
            error = "Actor monsterId not found: " + evt.actorId;
            return false;
        }
    } else {
        if (!ctx_.characterExists(evt.actorId)) {
            error = "Actor characterId not found: " + evt.actorId;
            return false;
        }
    }
    return true;
}

bool DndTxValidator::validateTargetExists(const DndEventTx& evt,
                                          std::string& error) const {
    if (!ctx_.characterExists || !ctx_.monsterExists) {
        return true;
    }

    const bool isMonster = (evt.targetType == 1);
    if (isMonster) {
        if (!ctx_.monsterExists(evt.targetId)) {
            error = "Target monsterId not found: " + evt.targetId;
            return false;
        }
    } else {
        if (!ctx_.characterExists(evt.targetId)) {
            error = "Target characterId not found: " + evt.targetId;
            return false;
        }
    }
    return true;
}

bool DndTxValidator::validateEncounter(const DndEventTx& evt,
                                       std::string& error) const {
    if (!ctx_.encounterActive) {
        return true;
    }
    if (!ctx_.encounterActive(evt.encounterId)) {
        error = "Encounter not active: " + evt.encounterId;
        return false;
    }
    return true;
}

bool DndTxValidator::validatePermissions(const DndEventTx& evt, std::string& error) const
{
    bool isMonster = (evt.actorType == 1);

    if (!ctx_.hasControlPermission(evt.actorId, evt.senderPubKey, isMonster)) {
        error = "permission denied for actorId=" + evt.actorId;
        return false;
    }

    return true;
}


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


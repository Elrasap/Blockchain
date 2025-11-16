#include "dnd/dndTxValidator.hpp"
#include <ctime>
#include <sstream>

namespace dnd {

DndTxValidator::DndTxValidator(DndValidationContext ctx)
    : ctx_(std::move(ctx)) {}

// ---------------------------------------------------------
//  Haupt-Entry: validate()
// ---------------------------------------------------------

bool DndTxValidator::validate(const DndEventTx& evt, std::string& error) const
{
    // 1) Actor / Target / Encounter
    if (!validateActorExists(evt, error))    return false;
    if (!validateTargetExists(evt, error))   return false;
    if (!validateEncounter(evt, error))      return false;

    // 2) Berechtigungen (Player vs DM)
    if (!validatePermissions(evt, error))    return false;

    // 3) Zeitkonsistenz
    if (!validateTimestamp(evt, error))      return false;

    return true;
}

// ---------------------------------------------------------
//  Actor-Existenz
// ---------------------------------------------------------

bool DndTxValidator::validateActorExists(const DndEventTx& evt,
                                         std::string& error) const
{
    if (!ctx_.characterExists && !ctx_.monsterExists) {
        // Wenn nichts verdrahtet ist, überspringen wir die Prüfung (für frühe Dev-Phase).
        return true;
    }

    const bool isMonster = (evt.actorType == 1);

    if (isMonster) {
        if (!ctx_.monsterExists) return true; // nichts verdrahtet
        if (!ctx_.monsterExists(evt.actorId)) {
            error = "Actor monster does not exist: " + evt.actorId;
            return false;
        }
    } else {
        if (!ctx_.characterExists) return true; // nichts verdrahtet
        if (!ctx_.characterExists(evt.actorId)) {
            error = "Actor character does not exist: " + evt.actorId;
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------
//  Target-Existenz
// ---------------------------------------------------------

bool DndTxValidator::validateTargetExists(const DndEventTx& evt,
                                          std::string& error) const
{
    if (evt.targetId.empty()) {
        // z.B. reine "Initiative-Rolls" könnten kein Target haben.
        return true;
    }

    if (!ctx_.characterExists && !ctx_.monsterExists) {
        return true;
    }

    const bool isMonster = (evt.targetType == 1);

    if (isMonster) {
        if (!ctx_.monsterExists) return true;
        if (!ctx_.monsterExists(evt.targetId)) {
            error = "Target monster does not exist: " + evt.targetId;
            return false;
        }
    } else {
        if (!ctx_.characterExists) return true;
        if (!ctx_.characterExists(evt.targetId)) {
            error = "Target character does not exist: " + evt.targetId;
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------
//  Encounter-Status
// ---------------------------------------------------------

bool DndTxValidator::validateEncounter(const DndEventTx& evt,
                                       std::string& error) const
{
    if (!ctx_.encounterActive) {
        // Noch nichts verdrahtet → überspringen
        return true;
    }

    if (evt.encounterId.empty()) {
        error = "Missing encounterId on DnD event";
        return false;
    }

    if (!ctx_.encounterActive(evt.encounterId)) {
        error = "Encounter is not active: " + evt.encounterId;
        return false;
    }

    return true;
}

// ---------------------------------------------------------
//  Berechtigungen
// ---------------------------------------------------------

bool DndTxValidator::validatePermissions(const DndEventTx& evt,
                                         std::string& error) const
{
    if (!ctx_.hasControlPermission) {
        // Noch nicht verdrahtet → keine zusätzliche Prüfung
        return true;
    }

    const bool isMonsterActor = (evt.actorType == 1);

    if (!ctx_.hasControlPermission(evt.actorId, evt.senderPubKey, isMonsterActor)) {
        std::ostringstream oss;
        oss << "Sender not allowed to control actor "
            << evt.actorId << " (isMonster=" << isMonsterActor << ")";
        error = oss.str();
        return false;
    }

    return true;
}

// ---------------------------------------------------------
//  Timestamp
// ---------------------------------------------------------

bool DndTxValidator::validateTimestamp(const DndEventTx& evt,
                                       std::string& error) const
{
    uint64_t now = ctx_.nowOverride != 0
                   ? ctx_.nowOverride
                   : static_cast<uint64_t>(std::time(nullptr));

    if (evt.timestamp == 0) {
        // ganz alte Events oder Tests → skippen
        return true;
    }

    // zu weit in der Zukunft?
    if (evt.timestamp > now + ctx_.maxFutureSkewSec) {
        error = "Event timestamp is too far in the future";
        return false;
    }

    // zu alt?
    if (evt.timestamp + ctx_.maxPastAgeSec < now) {
        error = "Event timestamp is too old";
        return false;
    }

    return true;
}

} // namespace dnd


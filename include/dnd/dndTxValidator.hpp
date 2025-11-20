#pragma once
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

#include "dnd/dndTx.hpp"

namespace dnd {

/**
 * Kontext für die Validierung eines DnD-Events.
 * Alles hier sind Callbacks, die du im Main/Node an deine echten Services bindest.
 */
struct DndValidationContext {
    // Charakter-Existenz (nur für actorType/targetType == 0)
    std::function<bool(const std::string& /*characterId*/)> characterExists;

    // Monster-Existenz (nur für actorType/targetType == 1)
    std::function<bool(const std::string& /*monsterId*/)> monsterExists;

    // Encounter aktiv?
    std::function<bool(const std::string& /*encounterId*/)> encounterActive;

    // Darf senderPubKey für actorId handeln?
    // isMonster == true → typischerweise: nur DM-Key darf das
    std::function<bool(const std::string& /*actorId*/,
                       const std::vector<uint8_t>& /*senderPubKey*/,
                       bool /*isMonster*/)> hasControlPermission;

    // Zeit-Toleranzen (Sekunden)
    uint64_t maxFutureSkewSec   = 30;       // wie weit darf ein Event in der Zukunft liegen
    uint64_t maxPastAgeSec      = 5 * 3600; // wie alt darf ein Event maximal sein (z.B. 5h)

    // Optional: "jetzt" – wenn 0 → time(nullptr) wird verwendet
    uint64_t nowOverride = 0;
};

class DndTxValidator {
public:
    explicit DndTxValidator(DndValidationContext ctx);

    // Validiert die komplette DnD-Ereignis-TX.
    // Gibt bei Fehler false zurück und setzt 'error' mit einer Menschen-lesbaren Ursache.
    bool validate(const DndEventTx& evt, std::string& error) const;

private:
    DndValidationContext ctx_;

    // 🔹 NEU: reine "Regel-Logik" basierend auf eventType
    bool validateSemantic(const DndEventTx& evt, std::string& error) const;

    bool validateActorExists(const DndEventTx& evt, std::string& error) const;
    bool validateTargetExists(const DndEventTx& evt, std::string& error) const;
    bool validateEncounter(const DndEventTx& evt, std::string& error) const;
    bool validatePermissions(const DndEventTx& evt, std::string& error) const;
    bool validateTimestamp(const DndEventTx& evt, std::string& error) const;
};

} // namespace dnd


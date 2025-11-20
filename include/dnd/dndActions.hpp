#pragma once
#include <string>
#include "dnd/dndTx.hpp"

namespace dnd::actions {

// Saving Throw
DndEventTx makeSavingThrow(const std::string& encounterId,
                           const std::string& actorId,
                           const std::string& ability, // "DEX", "STR", ...
                           int roll,
                           int dc,
                           bool success);

// Spell Cast
DndEventTx makeSpellCast(const std::string& encounterId,
                         const std::string& casterId,
                         const std::string& targetId,
                         const std::string& spellName,
                         int spellLevel,
                         int damage,
                         bool hit);

// Status Effect (Buff/Debuff)
DndEventTx makeStatusEffect(const std::string& encounterId,
                            const std::string& sourceId,
                            const std::string& targetId,
                            const std::string& effectName,
                            int durationRounds,
                            const std::string& effectDataJson);

// Loot Drop
DndEventTx makeLootDrop(const std::string& encounterId,
                        const std::string& sourceId,
                        const std::string& targetId,
                        const std::string& itemName,
                        int quantity,
                        int goldValue);

} // namespace dnd::actions


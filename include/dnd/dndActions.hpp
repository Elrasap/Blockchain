#pragma once
#include <string>
#include "dnd/dndTx.hpp"

namespace dnd::actions {


DndEventTx makeSavingThrow(const std::string& encounterId,
                           const std::string& actorId,
                           const std::string& ability,
                           int roll,
                           int dc,
                           bool success);


DndEventTx makeSpellCast(const std::string& encounterId,
                         const std::string& casterId,
                         const std::string& targetId,
                         const std::string& spellName,
                         int spellLevel,
                         int damage,
                         bool hit);


DndEventTx makeStatusEffect(const std::string& encounterId,
                            const std::string& sourceId,
                            const std::string& targetId,
                            const std::string& effectName,
                            int durationRounds,
                            const std::string& effectDataJson);


DndEventTx makeLootDrop(const std::string& encounterId,
                        const std::string& sourceId,
                        const std::string& targetId,
                        const std::string& itemName,
                        int quantity,
                        int goldValue);

}


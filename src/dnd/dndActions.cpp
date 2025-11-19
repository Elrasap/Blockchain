#include "dnd/dndActions.hpp"
#include <nlohmann/json.hpp>
#include <ctime>

using json = nlohmann::json;

namespace dnd::actions {

static uint64_t nowSec()
{
    return static_cast<uint64_t>(time(nullptr));
}

DndEventTx makeSavingThrow(const std::string& encounterId,
                           const std::string& actorId,
                           const std::string& ability,
                           int roll,
                           int dc,
                           bool success)
{
    DndEventTx evt;
    evt.encounterId = encounterId;
    evt.actorId     = actorId;
    evt.targetId    = actorId;
    evt.actorType   = 0;
    evt.targetType  = 0;
    evt.roll        = roll;
    evt.hit         = success;
    evt.damage      = 0;
    evt.timestamp   = nowSec();

    json meta;
    meta["type"]    = "savingThrow";
    meta["ability"] = ability;
    meta["dc"]      = dc;
    meta["success"] = success;

    evt.note = meta.dump();
    return evt;
}

DndEventTx makeSpellCast(const std::string& encounterId,
                         const std::string& casterId,
                         const std::string& targetId,
                         const std::string& spellName,
                         int spellLevel,
                         int damage,
                         bool hit)
{
    DndEventTx evt;
    evt.encounterId = encounterId;
    evt.actorId     = casterId;
    evt.targetId    = targetId;
    evt.actorType   = 0;
    evt.targetType  = 1;  // Default: Monster
    evt.roll        = 0;
    evt.damage      = damage;
    evt.hit         = hit;
    evt.timestamp   = nowSec();

    json meta;
    meta["type"]       = "spell";
    meta["spellName"]  = spellName;
    meta["spellLevel"] = spellLevel;
    meta["damage"]     = damage;
    meta["hit"]        = hit;

    evt.note = meta.dump();
    return evt;
}

DndEventTx makeStatusEffect(const std::string& encounterId,
                            const std::string& sourceId,
                            const std::string& targetId,
                            const std::string& effectName,
                            int durationRounds,
                            const std::string& effectDataJson)
{
    DndEventTx evt;
    evt.encounterId = encounterId;
    evt.actorId     = sourceId;
    evt.targetId    = targetId;
    evt.actorType   = 0;
    evt.targetType  = 0;
    evt.roll        = 0;
    evt.damage      = 0;
    evt.hit         = true;
    evt.timestamp   = nowSec();

    json meta;
    meta["type"]           = "status";
    meta["effect"]         = effectName;
    meta["durationRounds"] = durationRounds;

    if (!effectDataJson.empty()) {
        try {
            meta["data"] = json::parse(effectDataJson);
        } catch (...) {
            meta["data"] = effectDataJson;
        }
    }

    evt.note = meta.dump();
    return evt;
}

DndEventTx makeLootDrop(const std::string& encounterId,
                        const std::string& sourceId,
                        const std::string& targetId,
                        const std::string& itemName,
                        int quantity,
                        int goldValue)
{
    DndEventTx evt;
    evt.encounterId = encounterId;
    evt.actorId     = sourceId;
    evt.targetId    = targetId;
    evt.actorType   = 1;  // Monster as source
    evt.targetType  = 0;  // Character as receiver
    evt.roll        = 0;
    evt.damage      = 0;
    evt.hit         = true;
    evt.timestamp   = nowSec();

    json meta;
    meta["type"]      = "loot";
    meta["itemName"]  = itemName;
    meta["quantity"]  = quantity;
    meta["goldValue"] = goldValue;

    evt.note = meta.dump();
    return evt;
}

} // namespace dnd::actions


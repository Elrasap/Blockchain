#pragma once
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

#include "dnd/dndTx.hpp"

namespace dnd {


struct DndValidationContext {

    std::function<bool(const std::string& /*characterId*/)> characterExists;


    std::function<bool(const std::string& /*monsterId*/)> monsterExists;


    std::function<bool(const std::string& /*encounterId*/)> encounterActive;



    std::function<bool(const std::string& /*actorId*/,
                       const std::vector<uint8_t>& /*senderPubKey*/,
                       bool /*isMonster*/)> hasControlPermission;


    uint64_t maxFutureSkewSec   = 30;
    uint64_t maxPastAgeSec      = 5 * 3600;


    uint64_t nowOverride = 0;
};

class DndTxValidator {
public:
    explicit DndTxValidator(DndValidationContext ctx);



    bool validate(const DndEventTx& evt, std::string& error) const;

private:
    DndValidationContext ctx_;


    bool validateSemantic(const DndEventTx& evt, std::string& error) const;

    bool validateActorExists(const DndEventTx& evt, std::string& error) const;
    bool validateTargetExists(const DndEventTx& evt, std::string& error) const;
    bool validateEncounter(const DndEventTx& evt, std::string& error) const;
    bool validatePermissions(const DndEventTx& evt, std::string& error) const;
    bool validateTimestamp(const DndEventTx& evt, std::string& error) const;
};

}


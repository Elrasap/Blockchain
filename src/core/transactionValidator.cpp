#include "core/transactionValidator.hpp"

#include "dnd/dndPayload.hpp"
#include "dnd/dndTxValidator.hpp"

#include <sodium.h>

#include <utility>

TransactionValidator::TransactionValidator(std::vector<uint8_t> dmPublicKey)
    : dmPublicKey_(std::move(dmPublicKey))
{
}

bool TransactionValidator::validate(const Transaction& tx,
                                    const dnd::DndState& state,
                                    std::string& error,
                                    bool checkTimestamp) const
{
    dnd::DndState projected = state;
    return validateAndApply(tx, projected, error, checkTimestamp);
}

bool TransactionValidator::validateAndApply(const Transaction& tx,
                                            dnd::DndState& state,
                                            std::string& error,
                                            bool checkTimestamp) const
{
    error.clear();

    if (tx.senderPubkey.size() != crypto_sign_PUBLICKEYBYTES) {
        error = "TX: invalid sender public key length";
        return false;
    }
    if (tx.signature.size() != crypto_sign_BYTES) {
        error = "TX: missing or invalid signature";
        return false;
    }
    if (tx.payload.empty() || tx.payload.size() > Transaction::MAX_PAYLOAD_SIZE) {
        error = "TX: invalid payload size";
        return false;
    }
    if (!tx.verifySignature()) {
        error = "TX: invalid signature";
        return false;
    }

    if (!dnd::isDndPayload(tx.payload))
        return true;

    dnd::DndEventTx event;
    try {
        event = dnd::decodeDndTx(tx.payload);
    } catch (const std::exception& ex) {
        error = std::string("DnD decode failed: ") + ex.what();
        return false;
    }

    event.senderPubKey = tx.senderPubkey;
    event.signature = tx.signature;

    dnd::DndValidationContext context;
    context.characterExists = [&](const std::string& id) {
        return state.characterExists(id);
    };
    context.monsterExists = [&](const std::string& id) {
        return state.monsterExists(id);
    };
    context.encounterExists = [&](const std::string& id) {
        return state.encounterExists(id);
    };
    context.encounterActive = [&](const std::string& id) {
        return state.encounterActive(id);
    };
    context.isDungeonMaster = [&](const std::vector<uint8_t>& sender) {
        return sender == dmPublicKey_;
    };
    context.hasControlPermission =
        [&](const std::string& actorId,
            const std::vector<uint8_t>& sender,
            bool isMonster) {
            if (sender == dmPublicKey_)
                return true;
            if (isMonster)
                return false;
            const auto* owner = state.characterOwner(actorId);
            return owner != nullptr && !owner->empty() && *owner == sender;
        };
    context.checkTimestamp = checkTimestamp;

    dnd::DndTxValidator validator(std::move(context));
    if (!validator.validate(event, error))
        return false;

    if (!state.apply(event, error)) {
        error = "DnD state transition failed: " + error;
        return false;
    }
    return true;
}

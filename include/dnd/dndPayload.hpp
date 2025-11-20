#pragma once

#include <vector>
#include <string>

#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "core/transaction.hpp"

namespace dnd {

// Prüft nur das Magic-Byte der Payload.
// Schnell und sicher, kein JSON mehr.
inline bool isDndPayload(const std::vector<uint8_t>& payload)
{
    return !payload.empty() && payload[0] == 0xD1;
}

// Extract Event aus einer Transaction.
// senderPubKey kommt aus der TX, NICHT aus der Payload.
// signature: wir nutzen nur die TX-Signatur, Event-signature bleibt leer.
inline DndEventTx extractDndEventTx(const Transaction& tx)
{
    auto evt = decodeDndTx(tx.payload);
    evt.senderPubKey = tx.senderPubkey;

    // Wir WOLLEN keine zweite Signaturebene mehr.
    // Die gültige Signatur liegt auf der Transaction.
    evt.signature.clear();

    return evt;
}

} // namespace dnd


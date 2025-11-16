#pragma once
#include <vector>
#include "dnd/dndTx.hpp"

// Prüft ob eine TX-Payload ein DnD-Event enthält
namespace dnd {

inline bool isDndPayload(const std::vector<uint8_t>& payload)
{
    if (payload.size() < 4)
        return false;

    // "DND1" Header
    return payload[0] == 'D' &&
           payload[1] == 'N' &&
           payload[2] == 'D' &&
           payload[3] == '1';
}

// Extrahiert Event aus kompletter Transaction
inline DndEventTx extractDndEventTx(const Transaction& tx)
{
    auto evt = decodeDndTx(tx.payload);
    evt.senderPubKey = tx.senderPubkey;
    evt.signature    = tx.signature;
    return evt;
}

} // namespace dnd


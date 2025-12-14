#pragma once

#include <vector>
#include <string>

#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "core/transaction.hpp"

namespace dnd {



inline bool isDndPayload(const std::vector<uint8_t>& payload)
{
    return !payload.empty() && payload[0] == 0xD1;
}




inline DndEventTx extractDndEventTx(const Transaction& tx)
{
    auto evt = decodeDndTx(tx.payload);
    evt.senderPubKey = tx.senderPubkey;



    evt.signature.clear();

    return evt;
}

}


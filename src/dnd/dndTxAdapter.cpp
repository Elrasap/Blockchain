#include "dnd/dndTxAdapter.hpp"
#include "core/serialization.hpp"
#include "dnd/dndTxCodec.hpp"

// ==========================================================
// Wrap → Event → Blockchain-Transaction
// ==========================================================

Transaction wrapDndTxIntoTransaction(const dnd::DndEventTx& evt)
{
    Transaction tx;
    tx.senderPubkey = evt.senderPubKey;

    // Encode as DnD-typed payload
    tx.payload = dnd::encodeDndTx(evt);

    tx.nonce = 0; // später verbessern
    tx.fee   = 0;

    return tx;
}

// ==========================================================
// Extract → Blockchain-TX → DnD Event
// ==========================================================

dnd::DndEventTx extractDndEventTx(const Transaction& tx)
{
    auto evt = dnd::decodeDndTx(tx.payload);
    evt.senderPubKey = tx.senderPubkey;
    evt.signature    = tx.signature;
    return evt;
}


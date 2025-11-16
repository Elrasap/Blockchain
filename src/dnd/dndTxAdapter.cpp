#include "dnd/dndTxAdapter.hpp"
#include "dnd/serialization.hpp"

Transaction wrapDndTxIntoTransaction(const dnd::DndEventTx& evt)
{
    Transaction tx;
    tx.senderPubkey = evt.senderPubKey;

    tx.payload = encodeDndTx(evt);

    tx.nonce = 0; // später pro Spieler
    tx.fee = 0;

    return tx;
}

dnd::DndEventTx extractDndEventTx(const Transaction& tx)
{
    auto evt = decodeDndTx(tx.payload);
    evt.senderPubKey = tx.senderPubkey;
    evt.signature = tx.signature;
    return evt;
}


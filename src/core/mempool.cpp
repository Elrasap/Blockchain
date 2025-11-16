#include "core/mempool.hpp"
#include "dnd/serialization.hpp"
#include "dnd/dndTx.hpp"
#include "core/transaction.hpp"
#include <iostream>

using namespace std;

Mempool::Mempool(dnd::DndTxValidator* dndValidator)
    : dndValidator_(dndValidator)
{
}

// =========================================================
// Validierte Transaktion in den Mempool aufnehmen
// =========================================================
bool Mempool::addTransactionValidated(const Transaction& tx, std::string& errMsg)
{
    // ---------- 1) Generische Signatur ----------
    if (!tx.verifySignature()) {
        errMsg = "TX: invalid signature";
        return false;
    }

    // ---------- 2) Leere Payload verhindern ----------
    if (tx.payload.empty()) {
        errMsg = "TX: empty payload";
        return false;
    }

    // ---------- 3) Optional: Replay-Schutz ----------
    for (const auto& t : pool) {
        if (t.hash() == tx.hash()) {
            errMsg = "TX already exists in mempool";
            return false;
        }
    }

    // ---------- 4) DnD-Events erkennen ----------
    if (dnd::isDndPayload(tx.payload)) {

        // ---- Payload → Event dekodieren ----
        auto evt = dnd::decodeDndTx(tx.payload);

        // Pubkey/Signature vom äußeren TX übernehmen
        evt.senderPubKey = tx.senderPubkey;
        evt.signature    = tx.signature;

        // ---- 4.1 Signatur prüfen ----
        if (!dnd::verifyDndEventSignature(evt, errMsg))
            return false;

        // ---- 4.2 Validator aufrufen ----
        if (!dndValidator_->validate(evt, errMsg))
            return false;
    }

    // ---------- 5) In Mempool speichern ----------
    {
        lock_guard<mutex> lock(mtx);
        pool.push_back(tx);
    }

    return true;
}

// =========================================================
// Alle TXs (kopiert) zurückgeben
// =========================================================
std::vector<Transaction> Mempool::getAll()
{
    lock_guard<mutex> lock(mtx);
    return pool;
}

// =========================================================
// Mempool leeren
// =========================================================
void Mempool::clear()
{
    lock_guard<mutex> lock(mtx);
    pool.clear();
}

// =========================================================
// Anzahl der TXs
// =========================================================
size_t Mempool::size() const
{
    lock_guard<mutex> lock(mtx);
    return pool.size();
}


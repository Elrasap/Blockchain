#include "core/mempool.hpp"
#include "dnd/dndTxValidator.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/isDndPayload.hpp"

#include <algorithm>

Mempool::Mempool(dnd::DndTxValidator* validator)
    : validator_(validator)
{}

bool Mempool::addTransactionValidated(const Transaction& tx, std::string& err)
{
    std::lock_guard<std::mutex> lock(mtx_);

    // --- 1) Duplicate detection ---
    auto h = tx.hash();
    std::string key = hashToStr(h);

    if (knownHashes_.count(key)) {
        err = "duplicate transaction";
        return false;
    }

    // --- 2) DnD validation ---
    if (validator_) {

        if (dnd::isDndPayload(tx.payload)) {
            // decode event
            dnd::DndEventTx evt = dnd::decodeDndTx(tx.payload);

            // attach sender + sig
            evt.senderPubKey = tx.senderPubkey;
            evt.signature    = tx.signature;

            // validator API = ALWAYS: validate(evt, err)
            if (!validator_->validate(evt, err)) {
                return false;
            }
        }
    }

    // --- 3) Accept ---
    txs_.push_back(tx);
    knownHashes_.insert(key);

    return true;
}

std::vector<Transaction> Mempool::getAll() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return txs_;
}

size_t Mempool::size() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return txs_.size();
}

void Mempool::clear()
{
    std::lock_guard<std::mutex> lock(mtx_);
    txs_.clear();
    knownHashes_.clear();
}

void Mempool::remove(const std::array<uint8_t,32>& hash)
{
    std::lock_guard<std::mutex> lock(mtx_);

    std::string key = hashToStr(hash);

    txs_.erase(
        std::remove_if(txs_.begin(), txs_.end(),
            [&](const auto& tx) {
                return tx.hash() == hash;
            }),
        txs_.end()
    );

    knownHashes_.erase(key);
}


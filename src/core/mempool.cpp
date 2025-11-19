#include "core/mempool.hpp"
#include "dnd/dndTxValidator.hpp"
#include "dnd/dndTxCodec.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

Mempool::Mempool(dnd::DndTxValidator* validator)
    : validator_(validator)
{
}

bool Mempool::addTransactionValidated(const Transaction& tx, std::string& err)
{
    // Doppelt?
    auto h = tx.hash();
    std::string key = hashToStr(h);

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (knownHashes_.count(key) != 0) {
            err = "duplicate tx";
            return false;
        }
    }

    // Signatur + Basis-Check
    if (!tx.verifySignature()) {
        err = "invalid signature";
        return false;
    }

    // Optional DnD-Validator
    if (validator_) {
        // Hier validieren wir das Event – falls es DnD ist
        if (dnd::isDndPayload(tx.payload)) {
            auto evt = dnd::decodeDndTx(tx.payload);
            evt.senderPubKey = tx.senderPubkey;
            evt.signature    = tx.signature;

            std::string vErr;
            if (!validator_->validate(evt, vErr)) {
                err = "DnD validation failed: " + vErr;
                return false;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx_);
        txs_.push_back(tx);
        knownHashes_.insert(key);
    }

    return true;
}

std::vector<Transaction> Mempool::getAll() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return txs_;
}

void Mempool::clear()
{
    std::lock_guard<std::mutex> lock(mtx_);
    txs_.clear();
    knownHashes_.clear();
}

void Mempool::remove(const std::array<uint8_t, 32>& hash)
{
    std::string key = hashToStr(hash);

    std::lock_guard<std::mutex> lock(mtx_);

    auto it = std::remove_if(txs_.begin(), txs_.end(),
                             [&](const Transaction& tx) {
                                 return hashToStr(tx.hash()) == key;
                             });
    txs_.erase(it, txs_.end());
    knownHashes_.erase(key);
}

size_t Mempool::size() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return txs_.size();
}

// ======================================================
// Persistente Mempool-Datei
// ======================================================

bool Mempool::saveToFile(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(mtx_);

    json j = json::array();

    for (const auto& tx : txs_) {
        json t;
        t["payload"]      = tx.payload;
        t["senderPubKey"] = tx.senderPubkey;
        t["signature"]    = tx.signature;
        j.push_back(t);
    }

    try {
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            std::cerr << "[Mempool] saveToFile failed: cannot open " << path << "\n";
            return false;
        }
        out << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Mempool] saveToFile exception: " << e.what() << "\n";
        return false;
    }
}

bool Mempool::loadFromFile(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mtx_);

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        // Kein Fehler → Datei existiert evtl. einfach noch nicht
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        std::cerr << "[Mempool] loadFromFile parse error: " << e.what() << "\n";
        return false;
    }

    txs_.clear();
    knownHashes_.clear();

    for (const auto& t : j) {
        Transaction tx;
        tx.payload      = t.value("payload",      std::vector<uint8_t>{});
        tx.senderPubkey = t.value("senderPubKey", std::vector<uint8_t>{});
        tx.signature    = t.value("signature",    std::vector<uint8_t>{});

        txs_.push_back(tx);
        knownHashes_.insert(hashToStr(tx.hash()));
    }

    std::cout << "[Mempool] Restored " << txs_.size()
              << " transactions from " << path << "\n";

    return true;
}


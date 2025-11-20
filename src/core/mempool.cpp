#include "core/mempool.hpp"
#include "dnd/dndTxValidator.hpp"
#include "dnd/dndTxCodec.hpp"
#include <sodium.h>

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
    // Hash als String
    auto h   = tx.hash();
    std::string key = hashToStr(h);

    // Doppelte TX vermeiden
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (knownHashes_.count(key) != 0) {
            err = "duplicate tx";
            return false;
        }
    }

    // ==========================
    // 1) Signaturcheck (defensiv)
    // ==========================
    if (!tx.signature.empty()) {
        // libsodium: crypto_sign_BYTES ist erwartete Länge
        if (tx.signature.size() != crypto_sign_BYTES) {
            std::cerr << "[Mempool] invalid signature length: "
                      << tx.signature.size() << "\n";
            err = "invalid signature length";
            return false;
        }

        try {
            if (!tx.verifySignature()) {
                std::cerr << "[Mempool] verifySignature() failed\n";
                err = "invalid signature";
                return false;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[Mempool] verifySignature threw: "
                      << e.what() << "\n";
            err = "signature verify exception";
            return false;
        }
        catch (...) {
            std::cerr << "[Mempool] verifySignature threw unknown exception\n";
            err = "signature verify exception";
            return false;
        }
    } else {
        // Für jetzt erlauben wir uns das in Test-Phase;
        // wenn du später nur signierte TXs willst, hier "return false" machen.
        std::cerr << "[Mempool] WARNING: tx without signature accepted (test mode)\n";
    }

    // ==========================
    // 2) Optionaler DnD-Validator
    // ==========================
    if (validator_) {
        if (dnd::isDndPayload(tx.payload)) {
            dnd::DndEventTx evt;
            try {
                evt = dnd::decodeDndTx(tx.payload);
            }
            catch (const std::exception& e) {
                std::cerr << "[Mempool] decodeDndTx exception: "
                          << e.what() << "\n";
                err = "invalid dnd payload";
                return false;
            }
            catch (...) {
                std::cerr << "[Mempool] decodeDndTx unknown exception\n";
                err = "invalid dnd payload";
                return false;
            }

            // Sender/Signatur aus der TX übernehmen
            evt.senderPubKey = tx.senderPubkey;
            evt.signature    = tx.signature;

            std::string vErr;
            if (!validator_->validate(evt, vErr)) {
                std::cerr << "[Mempool] DnD validation failed: "
                          << vErr << "\n";
                err = "DnD validation failed: " + vErr;
                return false;
            }
        }
    }

    // ==========================
    // 3) Aufnahme in Mempool
    // ==========================
    {
        std::lock_guard<std::mutex> lock(mtx_);
        txs_.push_back(tx);
        knownHashes_.insert(key);
    }

    std::cout << "[Mempool] added tx " << key << "\n";
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
            std::cerr << "[Mempool] saveToFile failed: cannot open "
                      << path << "\n";
            return false;
        }
        out << j.dump(2);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[Mempool] saveToFile exception: "
                  << e.what() << "\n";
        return false;
    }
}

bool Mempool::loadFromFile(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mtx_);

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        // Kein Fehler → Datei existiert evtl. noch nicht
        return false;
    }

    json j;
    try {
        in >> j;
    }
    catch (const std::exception& e) {
        std::cerr << "[Mempool] loadFromFile parse error: "
                  << e.what() << "\n";
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


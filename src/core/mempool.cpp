#include "core/mempool.hpp"
#include "core/transactionValidator.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

using json = nlohmann::json;

Mempool::Mempool(
    const TransactionValidator& validator,
    std::function<dnd::DndState()> stateProvider,
    std::function<bool(const std::array<uint8_t, 32>&)> committedProvider)
    : validator_(validator),
      stateProvider_(std::move(stateProvider)),
      committedProvider_(std::move(committedProvider))
{
}

std::string Mempool::hashToStr(const std::array<uint8_t, 32>& hash) const
{
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (auto byte : hash) {
        out.push_back(hex[(byte >> 4) & 0xF]);
        out.push_back(hex[byte & 0xF]);
    }
    return out;
}

bool Mempool::addTransactionValidated(const Transaction& tx, std::string& err)
{
    std::lock_guard<std::mutex> admissionLock(admissionMutex_);
    std::array<uint8_t, 32> hash{};
    try {
        hash = tx.hash();
    } catch (const std::exception& ex) {
        err = ex.what();
        return false;
    }
    const std::string key = hashToStr(hash);

    std::vector<Transaction> pending;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (knownHashes_.contains(key)) {
            err = "duplicate tx";
            return false;
        }
        pending = txs_;
    }

    if (committedProvider_ && committedProvider_(hash)) {
        err = "transaction already committed";
        return false;
    }

    dnd::DndState projected = stateProvider_ ? stateProvider_() : dnd::DndState{};
    for (const auto& accepted : pending) {
        if (committedProvider_ && committedProvider_(accepted.hash()))
            continue;
        std::string ignored;
        if (!validator_.validateAndApply(accepted, projected, ignored, false)) {
            err = "mempool contains invalid dependency: " + ignored;
            return false;
        }
    }

    if (!validator_.validateAndApply(tx, projected, err, true))
        return false;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (knownHashes_.contains(key)) {
            err = "duplicate tx";
            return false;
        }
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
    std::lock_guard<std::mutex> admissionLock(admissionMutex_);
    std::lock_guard<std::mutex> lock(mtx_);
    txs_.clear();
    knownHashes_.clear();
}

void Mempool::remove(const std::array<uint8_t, 32>& hash)
{
    std::lock_guard<std::mutex> admissionLock(admissionMutex_);
    const std::string key = hashToStr(hash);
    std::lock_guard<std::mutex> lock(mtx_);
    txs_.erase(std::remove_if(txs_.begin(), txs_.end(),
        [&](const Transaction& tx) { return hashToStr(tx.hash()) == key; }),
        txs_.end());
    knownHashes_.erase(key);
}

void Mempool::remove(const std::vector<Transaction>& transactions)
{
    for (const auto& tx : transactions)
        remove(tx.hash());
}

size_t Mempool::size() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return txs_.size();
}

bool Mempool::saveToFile(const std::string& path) const
{
    json root = json::array();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (const auto& tx : txs_) {
            root.push_back({
                {"payload", tx.payload},
                {"senderPubKey", tx.senderPubkey},
                {"signature", tx.signature},
                {"nonce", tx.nonce},
                {"fee", tx.fee}
            });
        }
    }

    const std::filesystem::path target(path);
    const std::filesystem::path temporary = target.string() + ".tmp";
    try {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << root.dump(2);
        out.close();
        if (!out) return false;
        std::filesystem::rename(temporary, target);
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[Mempool] save failed: " << ex.what() << "\n";
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }
}

bool Mempool::loadFromFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    json root = json::parse(in, nullptr, false);
    if (!root.is_array()) {
        std::cerr << "[Mempool] invalid persisted mempool\n";
        return false;
    }

    clear();
    std::size_t restored = 0;
    for (const auto& entry : root) {
        try {
            Transaction tx;
            tx.payload = entry.value("payload", std::vector<uint8_t>{});
            tx.senderPubkey = entry.value("senderPubKey", std::vector<uint8_t>{});
            tx.signature = entry.value("signature", std::vector<uint8_t>{});
            tx.nonce = entry.value("nonce", uint64_t{0});
            tx.fee = entry.value("fee", uint64_t{0});

            std::string error;
            if (addTransactionValidated(tx, error)) {
                ++restored;
            } else {
                std::cerr << "[Mempool] skipped persisted transaction: "
                          << error << "\n";
            }
        } catch (const std::exception& ex) {
            std::cerr << "[Mempool] skipped malformed entry: "
                      << ex.what() << "\n";
        }
    }

    std::cout << "[Mempool] Restored " << restored
              << " validated transactions from " << path << "\n";
    return true;
}

#pragma once
#include <vector>
#include <mutex>
#include <array>
#include <unordered_set>
#include <string>

#include "core/transaction.hpp"

namespace dnd {
    class DndTxValidator;
}

class Mempool {
public:
    explicit Mempool(dnd::DndTxValidator* validator);
    bool ignoreSignatureCheck = false;
    // Validierender Insert
    bool addTransactionValidated(const Transaction& tx, std::string& err);

    // Alle TXs holen (Kopie)
    std::vector<Transaction> getAll() const;

    // Nach Mining gesamte Mempool leeren
    void clear();

    // Remove by hash (z.B. nach Block-Annahme)
    void remove(const std::array<uint8_t, 32>& hash);

    // Größe
    size_t size() const;

    // Persistenz – NICE-TO-HAVE
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

private:
    mutable std::mutex mtx_;
    std::vector<Transaction> txs_;
    std::unordered_set<std::string> knownHashes_; // duplicate filter

    dnd::DndTxValidator* validator_ = nullptr;

    std::string hashToStr(const std::array<uint8_t, 32>& h) const {
        // simple hex string
        static const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve(64);
        for (auto b : h) {
            out.push_back(hex[(b >> 4) & 0xF]);
            out.push_back(hex[b & 0xF]);
        }
        return out;
    }
};


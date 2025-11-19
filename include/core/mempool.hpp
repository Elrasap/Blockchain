#pragma once
#include <vector>
#include <mutex>
#include <unordered_set>
#include <array>
#include <string>

#include "core/transaction.hpp"

namespace dnd {
    class DndTxValidator;
}

class Mempool {
public:
    explicit Mempool(dnd::DndTxValidator* validator);

    bool addTransactionValidated(const Transaction& tx, std::string& err);
    std::vector<Transaction> getAll() const;
    size_t size() const;

    void clear();
    void remove(const std::array<uint8_t,32>& hash);

private:
    dnd::DndTxValidator* validator_;

    mutable std::mutex mtx_;
    std::vector<Transaction> txs_;

    std::unordered_set<std::string> knownHashes_;

    std::string hashToStr(const std::array<uint8_t, 32>& h) const {
        return std::string(reinterpret_cast<const char*>(h.data()), 32);
    }
};


#pragma once

#include <array>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/transaction.hpp"
#include "dnd/dndState.hpp"

class TransactionValidator;

class Mempool {
public:
    Mempool(const TransactionValidator& validator,
            std::function<dnd::DndState()> stateProvider,
            std::function<bool(const std::array<uint8_t, 32>&)> committedProvider = {});

    bool addTransactionValidated(const Transaction& tx, std::string& err);
    std::vector<Transaction> getAll() const;
    void clear();
    void remove(const std::array<uint8_t, 32>& hash);
    void remove(const std::vector<Transaction>& transactions);
    size_t size() const;

    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

private:
    mutable std::mutex mtx_;
    mutable std::mutex admissionMutex_;
    std::vector<Transaction> txs_;
    std::unordered_set<std::string> knownHashes_;

    const TransactionValidator& validator_;
    std::function<dnd::DndState()> stateProvider_;
    std::function<bool(const std::array<uint8_t, 32>&)> committedProvider_;

    std::string hashToStr(const std::array<uint8_t, 32>& hash) const;
};

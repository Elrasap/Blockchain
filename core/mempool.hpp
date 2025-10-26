#pragma once
#include <vector>
#include <array>
#include "crypto/types.hpp"
#include "core/transaction.hpp"

class Mempool {
public:
    void addTransaction(const Transaction& tx);
    std::vector<Transaction> getTransactions() const;
    void removeTransaction(const Transaction& tx);
    void purgeInvalidTransactions();
    uint64_t size() const;
};


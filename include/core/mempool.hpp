#pragma once
#include <vector>
#include <mutex>
#include "core/transaction.hpp"

class Mempool {
public:
    Mempool() = default;

    void addTransaction(const Transaction& tx);
    std::vector<Transaction> getAll() const;
    void clear();
    size_t size() const;

private:
    mutable std::mutex mtx;
    std::vector<Transaction> pending;
};


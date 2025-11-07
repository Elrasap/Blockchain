#pragma once
#include <vector>
#include <mutex>
#include "core/transaction.hpp"

class Mempool {
public:
    void addTransaction(const Transaction& tx);
    std::vector<Transaction> getAll();
    void clear();
    size_t size() const;

private:
    std::vector<Transaction> pool;
    mutable std::mutex mtx;
};


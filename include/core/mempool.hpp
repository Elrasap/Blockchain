#pragma once
#include <vector>
#include <mutex>
#include "core/transaction.hpp"
#include "dnd/dndTxValidator.hpp"

class Mempool {
public:
    Mempool(dnd::DndTxValidator* dndValidator);

    bool addTransactionValidated(const Transaction& tx, std::string& errMsg);

    std::vector<Transaction> getAll();
    void clear();
    size_t size() const;

private:
    std::vector<Transaction> pool;
    mutable std::mutex mtx;

    dnd::DndTxValidator* dndValidator_;  // neu
};


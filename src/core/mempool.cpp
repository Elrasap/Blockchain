#include "core/mempool.hpp"
#include "core/transaction.hpp"
#include <mutex>

void Mempool::addTransaction(const Transaction& tx) {
    std::lock_guard<std::mutex> lock(mtx);
    pending.push_back(tx);
}

std::vector<Transaction> Mempool::getAll() const {
    std::lock_guard<std::mutex> lock(mtx);
    return pending;  // sichere Kopie
}

void Mempool::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    pending.clear();
}

size_t Mempool::size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return pending.size();
}


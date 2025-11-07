#include "core/mempool.hpp"

using namespace std;

void Mempool::addTransaction(const Transaction& tx) {
    std::lock_guard<std::mutex> lock(mtx);
    pool.push_back(tx);
}

std::vector<Transaction> Mempool::getAll() {
    std::lock_guard<std::mutex> lock(mtx);
    return pool;
}

void Mempool::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    pool.clear();
}

size_t Mempool::size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return pool.size();
}


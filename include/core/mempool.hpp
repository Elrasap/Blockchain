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

    bool addTransactionValidated(const Transaction& tx, std::string& err);


    std::vector<Transaction> getAll() const;


    void clear();


    void remove(const std::array<uint8_t, 32>& hash);


    size_t size() const;


    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

private:
    mutable std::mutex mtx_;
    std::vector<Transaction> txs_;
    std::unordered_set<std::string> knownHashes_;

    dnd::DndTxValidator* validator_ = nullptr;

    std::string hashToStr(const std::array<uint8_t, 32>& h) const {

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


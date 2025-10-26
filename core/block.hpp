#pragma once
#include <vector>
#include <string>
#include "crypto/types.hpp"
#include "crypto/hash.hpp"
#include "transaction.hpp"

using namespace std;

class BlockHeader {
public:
    B32 prevHash;
    B32 merkelRoot;
    uint64_t height;
    uint64_t timestamp;
    uint64_t consensusTag;
    B32 nonceOrTerm;
    string hashAlgoId;

    vector<uint8_t> serializeHeader() const;
    B32 calcMerkelRoot(const vector<B32>& txHashes) const;
    B32 hash(IHashAlgorithm& hasher) const;
    string toString() const;
};

class Block {
public:
    BlockHeader header;
    uint64_t transactionCount() const { return transactions.size(); }
    vector<Transaction> transactions;
};

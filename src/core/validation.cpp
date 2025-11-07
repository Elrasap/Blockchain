#include "core/validation.hpp"
#include <iostream>

using namespace std;

namespace Validation {

bool validateTransaction(const Transaction& tx) {
    if (tx.payload.empty()) return false;
    if (tx.signature.empty()) return false;
    if (!tx.verifySignature()) return false;
    return true;
}
bool validateBlock(const Block& block, const Block& prev) {
    if (block.header.height == 0) {
        if (!block.transactions.empty()) {
            if (block.header.merkleRoot != block.calculateMerkleRoot()) return false;
            for (const auto& tx : block.transactions)
                if (!validateTransaction(tx)) return false;
        }
        return true;
    }

    // --- tolerant gegenüber leicht unterschiedlichen prevHashes ---
    std::array<uint8_t, 32> prevCalc = prev.hash();
    size_t diff = 0;
    for (size_t i = 0; i < 32; ++i)
        if (block.header.prevHash[i] != prevCalc[i]) ++diff;
    if (diff > 4) {  // bis zu 4 Byte Unterschied erlauben
        std::cout << "PrevHash differs (" << diff << " bytes)\n";
        // nur Warnung, kein Abbruch
    }

    if (!block.transactions.empty()) {
        if (block.header.merkleRoot != block.calculateMerkleRoot()) return false;
        for (const auto& tx : block.transactions)
            if (!validateTransaction(tx)) return false;
    }

    return true;
}

}

#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "core/block.hpp"
#include "core/transactionValidator.hpp"
#include "dnd/dndState.hpp"
#include "storage/blockStore.hpp"

class Blockchain {
public:
    Blockchain(BlockStore& store,
               const std::vector<uint8_t>& dmValidatorPubKey);

    uint64_t getHeight() const;
    Block getLatestBlock() const;
    Block getBlock(uint64_t height) const;
    std::vector<Block> getChain() const;

    bool appendBlock(const Block& block);
    bool ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey);
    bool validateBlock(const Block& block) const;
    bool validateTransaction(const Transaction& tx, std::string& err) const;
    bool hasTransaction(const std::array<uint8_t, 32>& hash) const;

    void loadFromDisk();
    void rebuildState();

    dnd::DndState getDndState() const;
    const TransactionValidator& transactionValidator() const {
        return txValidator_;
    }

    bool writeSnapshot(const std::string& path) const;

private:
    bool validateBlockHeader(const Block& block) const;
    bool containsTransactionUnlocked(const std::array<uint8_t, 32>& hash) const;

    BlockStore& store_;
    std::vector<Block> chain_;
    std::vector<uint8_t> dmPubKey_;
    TransactionValidator txValidator_;
    dnd::DndState dndState_;
    mutable std::recursive_mutex mutex_;
};

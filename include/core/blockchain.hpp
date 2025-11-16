#pragma once
#include <vector>
#include <cstdint>
#include <memory>

#include "core/block.hpp"
#include "storage/blockStore.hpp"

#include "dnd/state.hpp"
#include "dnd/stateSnapshot.hpp"

class Blockchain {
public:
    Blockchain(BlockStore& store,
               const std::vector<uint8_t>& dmValidatorPubKey);

    //
    // Chain info
    //
    uint64_t getHeight() const;
    Block getLatestBlock() const;
    Block getBlock(uint64_t height) const;
    const std::vector<Block>& getChain() const { return chain_; }

    //
    // Block operations
    //
    bool appendBlock(const Block& block);
    bool ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey);

    //
    // Validation
    //
    bool validateBlock(const Block& block) const;

    //
    // Transaction validation
    //
    bool validateTransaction(const Transaction& tx,
                             std::string& err) const;

    //
    // Startup actions
    //
    void loadFromDisk();              // 6.2 – lädt & validiert
    void rebuildState();              // 6.2 – nutzt State::apply()

    //
    // Access DnD world state
    //
    const dnd::DndState& getDndState() const { return dndState_; }

    //
    // 6.3 Snapshot
    //
    bool writeSnapshot(const std::string& path) const;
    bool loadSnapshot(const std::string& path);

private:
    BlockStore& store_;
    std::vector<Block> chain_;
    std::vector<uint8_t> dmPubKey;

    dnd::DndState dndState_;
};


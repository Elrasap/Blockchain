#pragma once
#include <vector>
#include <cstdint>
#include <memory>

#include "core/block.hpp"
#include "storage/blockStore.hpp"

#include "dnd/state.hpp"           // <-- wichtig: EINZIGE Dnd-State-Definition
#include "dnd/stateSnapshot.hpp"

class Blockchain {
public:
    Blockchain(BlockStore& store,
               const std::vector<uint8_t>& dmValidatorPubKey);

    // Chain info
    uint64_t getHeight() const;
    Block getLatestBlock() const;
    Block getBlock(uint64_t height) const;
    const std::vector<Block>& getChain() const { return chain_; }

    // Block operations
    bool appendBlock(const Block& block);
    bool ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey);

    // Validation
    bool validateBlock(const Block& block) const;

    // Transaction validation
    bool validateTransaction(const Transaction& tx,
                             std::string& err) const;

    // Startup actions
    void loadFromDisk();
    void rebuildState();

    // DnD state access
    const dnd::DndState& getDndState() const { return dndState_; }

    // Snapshots
    bool writeSnapshot(const std::string& path) const;
    bool loadSnapshot(const std::string& path);

private:
    BlockStore& store_;
    std::vector<Block> chain_;
    std::vector<uint8_t> dmPubKey;

    dnd::DndState dndState_;   // <-- DARF JETZT ERKANNT WERDEN
};


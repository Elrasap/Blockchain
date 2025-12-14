#pragma once
#include <vector>
#include <cstdint>
#include <memory>

#include "core/block.hpp"
#include "storage/blockStore.hpp"
#include "dnd/dndState.hpp"

#include "dnd/stateSnapshot.hpp"

class Blockchain {
public:
    Blockchain(BlockStore& store,
               const std::vector<uint8_t>& dmValidatorPubKey);


    uint64_t getHeight() const;
    Block getLatestBlock() const;
    Block getBlock(uint64_t height) const;
    const std::vector<Block>& getChain() const { return chain_; }


    bool appendBlock(const Block& block);
    bool ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey);


    bool validateBlock(const Block& block) const;


    bool validateTransaction(const Transaction& tx,
                             std::string& err) const;


    void loadFromDisk();
    void rebuildState();


    const dnd::DndState& getDndState() const { return dndState_; }


    bool writeSnapshot(const std::string& path) const;
    bool loadSnapshot(const std::string& path);

private:
    BlockStore& store_;
    std::vector<Block> chain_;
    std::vector<uint8_t> dmPubKey;

    dnd::DndState dndState_;
};


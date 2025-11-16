#include "core/blockchain.hpp"
#include "core/poaValidator.hpp"
#include "core/blockJson.hpp"

#include <iostream>
#include <ctime>

// =====================================================
// Constructor
// =====================================================

Blockchain::Blockchain(BlockStore& store,
                       const std::vector<uint8_t>& dmValidatorPubKey)
    : store_(store), dmPubKey(dmValidatorPubKey)
{
    loadFromDisk();
    rebuildState();
}

// =====================================================
// Load blocks from disk
// =====================================================

void Blockchain::loadFromDisk() {
    std::cout << "[Blockchain] Loading blocks from disk...\n";

    chain_ = store_.loadAllBlocks();

    if (chain_.empty()) {
        std::cout << "[Blockchain] No blocks found.\n";
        return;
    }

    // validate sequentially
    for (size_t i = 1; i < chain_.size(); i++) {
        const auto& prev = chain_[i - 1];
        const auto& curr = chain_[i];

        if (curr.header.height != prev.header.height + 1) {
            std::cerr << "[Blockchain] Height mismatch at block "
                      << i << "\n";
            chain_.resize(i);
            break;
        }

        if (curr.header.prevHash != prev.hash()) {
            std::cerr << "[Blockchain] prevHash mismatch at block "
                      << i << "\n";
            chain_.resize(i);
            break;
        }

        if (!validateBlock(curr)) {
            std::cerr << "[Blockchain] Invalid block at height "
                      << curr.header.height << "\n";
            chain_.resize(i);
            break;
        }
    }

    std::cout << "[Blockchain] Loaded "
              << chain_.size() << " valid blocks.\n";
}

// =====================================================
// Rebuild DnD state from chain
// =====================================================

void Blockchain::rebuildState() {
    std::cout << "[DndState] Rebuilding...\n";

    dndState_.clear();

    for (const auto& block : chain_) {
        for (const auto& tx : block.transactions) {
            if (!dnd::isDndPayload(tx.payload))
                continue;

            auto evt = dnd::extractDndEventTx(tx);
            std::string err;

            // Full DnD validation is optional here (already validated on block add)
            if (!dndState_.apply(evt, err)) {
                std::cerr << "[DndState] apply() failed at block "
                          << block.header.height << ": " << err << "\n";
            }
        }
    }

    std::cout << "[DndState] Rebuild complete. Characters: "
              << dndState_.characters.size()
              << " Monsters: " << dndState_.monsters.size()
              << " Encounters: " << dndState_.encounters.size()
              << "\n";
}

// =====================================================
// Height, read
// =====================================================

uint64_t Blockchain::getHeight() const {
    if (chain_.empty()) return 0;
    return chain_.back().header.height;
}

Block Blockchain::getLatestBlock() const {
    if (chain_.empty()) return Block{};
    return chain_.back();
}

Block Blockchain::getBlock(uint64_t height) const {
    if (height >= chain_.size()) return Block{};
    return chain_[height];
}

// =====================================================
// Validate block
// =====================================================

bool Blockchain::validateBlock(const Block& block) const {
    const BlockHeader& h = block.header;

    // PoA check
    if (h.validatorPubKey != dmPubKey) {
        std::cerr << "[PoA] Unauthorized validator\n";
        return false;
    }

    if (!verifyBlockHeaderSignature(h)) {
        std::cerr << "[PoA] Invalid signature\n";
        return false;
    }

    // Merkle root
    auto calc = block.calculateMerkleRoot();
    if (calc != h.merkleRoot) {
        std::cerr << "[PoA] Invalid Merkle root\n";
        return false;
    }

    // Time sanity
    uint64_t now = time(nullptr);
    if (h.timestamp > now + 30) {
        std::cerr << "[PoA] Timestamp too far in future\n";
        return false;
    }

    return true;
}

// =====================================================
// Validate Transaction
// =====================================================

bool Blockchain::validateTransaction(const Transaction& tx,
                                     std::string& err) const
{
    if (!tx.verifySignature()) {
        err = "TX: invalid signature";
        return false;
    }

    if (dnd::isDndPayload(tx.payload)) {
        auto evt = dnd::decodeDndTx(tx.payload);
        evt.senderPubKey = tx.senderPubkey;
        evt.signature    = tx.signature;

        if (!dndState_.validate(evt, err))
            return false;
    }

    return true;
}

// =====================================================
// Append Block
// =====================================================

bool Blockchain::appendBlock(const Block& block) {
    if (!validateBlock(block)) return false;

    if (!store_.appendBlock(block)) {
        std::cerr << "[Blockchain] Store append failed\n";
        return false;
    }

    chain_.push_back(block);

    // apply all block txs to state
    for (const auto& tx : block.transactions) {
        if (!dnd::isDndPayload(tx.payload))
            continue;

        auto evt = dnd::extractDndEventTx(tx);
        std::string err;

        if (!dndState_.apply(evt, err)) {
            std::cerr << "[DndState] appendBlock apply fail: " << err << "\n";
        }
    }

    return true;
}

// =====================================================
// Genesis
// =====================================================

bool Blockchain::ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey) {
    if (!chain_.empty()) return true;

    Block genesis;
    genesis.header.height = 0;
    genesis.header.timestamp = time(nullptr);
    genesis.header.prevHash.fill(0);
    genesis.header.merkleRoot.fill(0);

    signBlockHeader(genesis.header, dmPrivKey, dmPubKey);

    if (!store_.appendBlock(genesis)) return false;

    chain_.push_back(genesis);
    return true;
}

// =====================================================
// Snapshot Write
// =====================================================

bool Blockchain::writeSnapshot(const std::string& path) const {
    return dndState_.writeSnapshot(path);
}

// =====================================================
// Snapshot Load
// =====================================================

bool Blockchain::loadSnapshot(const std::string& path) {
    if (!dndState_.loadSnapshot(path))
        return false;

    return true;
}


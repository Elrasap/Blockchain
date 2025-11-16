#include "core/blockchain.hpp"
#include "core/validation.hpp"
#include "core/crypto.hpp"

#include <chrono>
#include <cstring>
#include <iostream>

using namespace std;

namespace {

uint64_t nowMs() {
    using namespace std::chrono;
    auto now = system_clock::now().time_since_epoch();
    return duration_cast<milliseconds>(now).count();
}

bool isZeroHash(const std::array<uint8_t, 32>& h) {
    for (auto b : h) {
        if (b != 0) return false;
    }
    return true;
}

} // namespace

Blockchain::Blockchain(BlockStore& store,
                       const std::vector<uint8_t>& dmValidatorPubKey)
    : store(store),
      dmPubKey(dmValidatorPubKey) {}

uint64_t Blockchain::getHeight() const {
    Block latest = store.getLatestBlock();
    return latest.header.height;
}

Block Blockchain::getLatestBlock() const {
    return store.getLatestBlock();
}

Block Blockchain::getBlock(uint64_t height) const {
    auto all = store.loadAllBlocks();
    for (const auto& b : all) {
        if (b.header.height == height) {
            return b;
        }
    }
    return Block{}; // height = 0
}

std::vector<Block> Blockchain::loadChain() const {
    return store.loadAllBlocks();
}

bool Blockchain::validateBlock(const Block& block) const {
    // Genesis-Block: height == 0, kein prev
    if (block.header.height == 0) {
        // Optional: minimale Checks
        if (!verifyBlockHeaderSignature(block.header)) {
            return false;
        }
        if (block.header.validatorPubKey != dmPubKey) {
            return false;
        }
        return true;
    }

    auto all = store.loadAllBlocks();
    if (all.empty()) {
        // Kein vorheriger Block, aber height > 0 → ungültig
        return false;
    }

    // Vorgänger mit height-1 suchen
    Block prev;
    bool foundPrev = false;
    for (const auto& b : all) {
        if (b.header.height == block.header.height - 1) {
            prev = b;
            foundPrev = true;
            break;
        }
    }
    if (!foundPrev) {
        return false;
    }

    // Proof-of-Authority Validierung
    return Validation::validateBlockPoA(block, prev, dmPubKey);
}

bool Blockchain::appendBlock(const Block& block) {
    if (!validateBlock(block)) {
        std::cerr << "[Blockchain] Block validation failed at height "
                  << block.header.height << "\n";
        return false;
    }

    if (!store.appendBlock(block)) {
        std::cerr << "[Blockchain] Failed to append block to store.\n";
        return false;
    }

    return true;
}

bool Blockchain::ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey) {
    // Wenn schon irgendwas in der DB ist → nichts tun
    auto all = store.loadAllBlocks();
    if (!all.empty()) {
        return true;
    }

    Block genesis;
    genesis.header.height = 0;
    genesis.header.timestamp = nowMs();

    // prevHash = 0
    genesis.header.prevHash = {};
    // keine TXs → MerkleRoot über leere Liste
    genesis.header.merkleRoot = genesis.calculateMerkleRoot();

    // Signieren mit DM-Key
    if (!signBlockHeader(genesis.header, dmPrivKey, dmPubKey)) {
        std::cerr << "[Blockchain] Failed to sign genesis block.\n";
        return false;
    }

    if (!store.appendBlock(genesis)) {
        std::cerr << "[Blockchain] Failed to append genesis block.\n";
        return false;
    }

    std::cout << "[Blockchain] Genesis block created.\n";
    return true;
}


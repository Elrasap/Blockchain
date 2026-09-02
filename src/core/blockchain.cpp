#include "core/blockchain.hpp"

#include "core/crypto.hpp"
#include "core/poaValidator.hpp"
#include "dnd/dndPayload.hpp"
#include "dnd/stateSnapshot.hpp"

#include <ctime>
#include <iostream>
#include <unordered_set>

namespace {

std::string hashKey(const std::array<uint8_t, 32>& hash)
{
    return crypto::toHex(hash);
}

} // namespace

Blockchain::Blockchain(BlockStore& store,
                       const std::vector<uint8_t>& dmValidatorPubKey)
    : store_(store),
      dmPubKey_(dmValidatorPubKey),
      txValidator_(dmValidatorPubKey)
{
    loadFromDisk();
}

void Blockchain::loadFromDisk()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto stored = store_.loadAllBlocks();

    chain_.clear();
    dndState_.clear();
    dnd::DndState projected;
    std::unordered_set<std::string> transactionHashes;

    for (std::size_t i = 0; i < stored.size(); ++i) {
        const Block& block = stored[i];
        const uint64_t expectedHeight = static_cast<uint64_t>(chain_.size());

        if (block.header.height != expectedHeight ||
            (expectedHeight == 0 && block.header.prevHash != std::array<uint8_t, 32>{}) ||
            (expectedHeight > 0 && block.header.prevHash != chain_.back().hash()) ||
            !validateBlockHeader(block)) {
            std::cerr << "[Blockchain] Ignoring invalid stored chain from height "
                      << block.header.height << "\n";
            break;
        }

        bool validTransactions = true;
        dnd::DndState blockProjection = projected;
        std::unordered_set<std::string> blockHashes;
        for (const auto& tx : block.transactions) {
            std::string error;
            std::array<uint8_t, 32> hash{};
            try {
                hash = tx.hash();
            } catch (const std::exception& ex) {
                error = ex.what();
            }
            const std::string key = hashKey(hash);
            if (!error.empty() || transactionHashes.contains(key) ||
                !blockHashes.insert(key).second ||
                !txValidator_.validateAndApply(tx, blockProjection, error, false)) {
                std::cerr << "[Blockchain] Invalid stored transaction at height "
                          << block.header.height << ": " << error << "\n";
                validTransactions = false;
                break;
            }
        }
        if (!validTransactions)
            break;

        projected = std::move(blockProjection);
        transactionHashes.insert(blockHashes.begin(), blockHashes.end());
        chain_.push_back(block);
    }

    dndState_ = std::move(projected);
    std::cout << "[Blockchain] Loaded " << chain_.size()
              << " validated blocks from disk\n";
}

void Blockchain::rebuildState()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    dnd::DndState projected;

    for (const auto& block : chain_) {
        for (const auto& tx : block.transactions) {
            std::string error;
            if (!txValidator_.validateAndApply(tx, projected, error, false)) {
                std::cerr << "[DndState] Replay failed at height "
                          << block.header.height << ": " << error << "\n";
                return;
            }
        }
    }
    dndState_ = std::move(projected);
}

uint64_t Blockchain::getHeight() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return chain_.empty() ? 0 : chain_.back().header.height;
}

Block Blockchain::getLatestBlock() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return chain_.empty() ? Block{} : chain_.back();
}

Block Blockchain::getBlock(uint64_t height) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return height < chain_.size() ? chain_[height] : Block{};
}

std::vector<Block> Blockchain::getChain() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return chain_;
}

dnd::DndState Blockchain::getDndState() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return dndState_;
}

bool Blockchain::validateBlockHeader(const Block& block) const
{
    const BlockHeader& header = block.header;
    if (header.validatorPubKey != dmPubKey_ ||
        !verifyBlockHeaderSignature(header)) {
        std::cerr << "[PoA] Unauthorized or invalid validator signature\n";
        return false;
    }
    if (block.transactions.size() > Block::MAX_TRANSACTION_COUNT ||
        header.merkleRoot != block.calculateMerkleRoot()) {
        std::cerr << "[PoA] Invalid block body commitment\n";
        return false;
    }
    const uint64_t now = static_cast<uint64_t>(std::time(nullptr));
    if (header.timestamp > now + 30) {
        std::cerr << "[PoA] Timestamp too far in future\n";
        return false;
    }
    return true;
}

bool Blockchain::validateBlock(const Block& block) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return validateBlockHeader(block);
}

bool Blockchain::validateTransaction(const Transaction& tx,
                                     std::string& err) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return txValidator_.validate(tx, dndState_, err, true);
}

bool Blockchain::containsTransactionUnlocked(
    const std::array<uint8_t, 32>& hash) const
{
    for (const auto& block : chain_)
        for (const auto& tx : block.transactions)
            if (tx.hash() == hash)
                return true;
    return false;
}

bool Blockchain::hasTransaction(const std::array<uint8_t, 32>& hash) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return containsTransactionUnlocked(hash);
}

bool Blockchain::appendBlock(const Block& block)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const uint64_t expectedHeight = static_cast<uint64_t>(chain_.size());

    if (block.header.height < expectedHeight) {
        if (block.header.height < chain_.size() &&
            chain_[block.header.height].hash() != block.hash()) {
            std::cerr << "[Consensus] Validator equivocation detected at height "
                      << block.header.height << "\n";
        }
        return false;
    }
    if (block.header.height > expectedHeight) {
        std::cerr << "[Consensus] Missing parent for block "
                  << block.header.height << "\n";
        return false;
    }

    const std::array<uint8_t, 32> expectedPrevious = chain_.empty()
        ? std::array<uint8_t, 32>{} : chain_.back().hash();
    if (block.header.prevHash != expectedPrevious)
        return false;
    if (!chain_.empty() && block.header.timestamp < chain_.back().header.timestamp)
        return false;
    if (!validateBlockHeader(block))
        return false;

    dnd::DndState projected = dndState_;
    std::unordered_set<std::string> blockHashes;
    for (const auto& tx : block.transactions) {
        std::string error;
        std::array<uint8_t, 32> hash{};
        try {
            hash = tx.hash();
        } catch (const std::exception& ex) {
            std::cerr << "[Blockchain] Invalid transaction: " << ex.what() << "\n";
            return false;
        }

        if (containsTransactionUnlocked(hash) ||
            !blockHashes.insert(hashKey(hash)).second) {
            std::cerr << "[Blockchain] Replayed transaction rejected\n";
            return false;
        }
        if (!txValidator_.validateAndApply(tx, projected, error, false)) {
            std::cerr << "[Blockchain] Transaction rejected: " << error << "\n";
            return false;
        }
    }

    if (!store_.appendBlock(block)) {
        std::cerr << "[Blockchain] Store append failed\n";
        return false;
    }

    chain_.push_back(block);
    dndState_ = std::move(projected);
    return true;
}

bool Blockchain::ensureGenesisBlock(const std::vector<uint8_t>& dmPrivKey)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!chain_.empty()) return true;

    Block genesis;
    genesis.header.height = 0;
    genesis.header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    genesis.header.prevHash.fill(0);
    genesis.header.merkleRoot = genesis.calculateMerkleRoot();

    if (!signBlockHeader(genesis.header, dmPrivKey, dmPubKey_) ||
        !validateBlockHeader(genesis) ||
        !store_.appendBlock(genesis))
        return false;

    chain_.push_back(std::move(genesis));
    return true;
}

bool Blockchain::writeSnapshot(const std::string& path) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return dnd::writeSnapshot(dndState_, path);
}

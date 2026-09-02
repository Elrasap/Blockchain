#include "network/syncManager.hpp"

#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "network/messages.hpp"
#include "network/peerManager.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

SyncManager::SyncManager(Blockchain& chain, PeerManager& peers, Mempool* mempool)
    : chain_(chain), peers_(peers), mempool_(mempool)
{
}

SyncManager::~SyncManager()
{
    stop();
}

void SyncManager::start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
        return;
    worker_ = std::thread(&SyncManager::loop, this);
}

void SyncManager::stop()
{
    if (!running_.exchange(false))
        return;
    if (worker_.joinable())
        worker_.join();
}

bool SyncManager::hasBlock(const std::array<uint8_t, 32>& hash) const
{
    for (const auto& block : chain_.getChain())
        if (block.hash() == hash)
            return true;
    return false;
}

void SyncManager::requestBlock(const std::array<uint8_t, 32>& hash)
{
    peers_.broadcast({MessageType::GETBLOCK, {hash.begin(), hash.end()}});
}

void SyncManager::announce(const Block& block)
{
    const auto hash = block.hash();
    peers_.broadcast({MessageType::INV, {hash.begin(), hash.end()}});
}

void SyncManager::sendTip(int peerFd)
{
    const Block tip = chain_.getLatestBlock();
    if (tip.header.signature.empty()) return;
    const auto hash = tip.hash();
    peers_.sendTo(peerFd, {MessageType::INV, {hash.begin(), hash.end()}});
}

void SyncManager::handleInv(const std::array<uint8_t, 32>& hash)
{
    if (hasBlock(hash)) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(wanted_.begin(), wanted_.end(), hash) == wanted_.end())
        wanted_.push_back(hash);
}

void SyncManager::handleGetBlock(const std::array<uint8_t, 32>& hash, int fd)
{
    for (const auto& block : chain_.getChain()) {
        if (block.hash() == hash) {
            peers_.sendTo(fd, {MessageType::BLOCK, block.serialize()});
            return;
        }
    }
}

bool SyncManager::handleBlock(const Block& block)
{
    const uint64_t expectedHeight = chain_.getChain().size();
    if (block.header.height < expectedHeight) {
        const Block known = chain_.getBlock(block.header.height);
        if (known.hash() != block.hash())
            chain_.appendBlock(block); // emits the equivocation diagnostic
        return known.hash() == block.hash();
    }

    if (!chain_.validateBlock(block))
        return false;

    if (block.header.height > expectedHeight) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto [it, inserted] = pending_.emplace(block.header.height, block);
            if (!inserted && it->second.hash() != block.hash()) {
                std::cerr << "[Sync] Conflicting pending blocks at height "
                          << block.header.height << "\n";
                return false;
            }
        }
        handleInv(block.header.prevHash);
        return false;
    }

    if (!chain_.appendBlock(block))
        return false;
    if (mempool_) mempool_->remove(block.transactions);
    announce(block);

    while (true) {
        Block next;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = pending_.find(chain_.getChain().size());
            if (it == pending_.end()) break;
            next = it->second;
            pending_.erase(it);
        }

        if (next.header.prevHash != chain_.getLatestBlock().hash() ||
            !chain_.appendBlock(next)) {
            std::cerr << "[Sync] Buffered block rejected at height "
                      << next.header.height << "\n";
            break;
        }
        if (mempool_) mempool_->remove(next.transactions);
        announce(next);
    }
    return true;
}

void SyncManager::loop()
{
    while (running_) {
        std::vector<std::array<uint8_t, 32>> wanted;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            wanted.swap(wanted_);
        }
        for (const auto& hash : wanted)
            if (!hasBlock(hash))
                requestBlock(hash);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

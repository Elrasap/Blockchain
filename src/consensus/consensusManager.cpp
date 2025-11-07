#include "consensus/consensusManager.hpp"
#include "network/messages.hpp"
#include "core/validation.hpp"
#include "storage/commitLog.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include "light/merkleProof.hpp"


using namespace std;

ConsensusManager::ConsensusManager(BlockStore* s, PeerManager* p, bool isLeader)
    : store(s),
      peers(p),
      leader(isLeader),
      commit_log(std::string("commit_consensus.dat")),
      snapshot_mgr(s),
      finality_mgr(s) {}

void ConsensusManager::start() {
    running = true;
    worker = thread([this]() {
        while (running) {
            tick();
            this_thread::sleep_for(chrono::milliseconds(199));
        }
    });
}

void ConsensusManager::stop() {
    running = false;
    if (worker.joinable()) worker.join();
}

void ConsensusManager::tick() {
    if (!leader) return;
    auto blocks = store->loadAllBlocks();
    if (!blocks.empty()) {
        uint64_t h = blocks.back().header.height;
        if (h % 10 == 0 && h != last_snapshot_height) {
            periodicSnapshot();
            last_snapshot_height = h;
        }
    }
}

void ConsensusManager::broadcastAppend(const Block& block) {
    Message msg;
    msg.type = MessageType::APPEND_ENTRIES;
    msg.payload = block.serialize();
    peers->broadcast(msg);
    ack_count[block.hash()] = 0;
}

void ConsensusManager::receiveAppend(const Block& block, int peer_fd) {
    if (Validation::validateBlock(block, store->getLatestBlock())) {
        store->appendBlock(block);
        Message ack;
        ack.type = MessageType::ACK;
        ack.payload.assign(block.hash().begin(), block.hash().end());
        peers->sendTo(peer_fd, ack);
    }
}


bool ConsensusManager::isCommitted(const array<uint8_t,32>& hash) {
    lock_guard<mutex> lock(mtx);
    return committed[hash];
}

void ConsensusManager::recoverCommitted() {
    auto entries = commit_log.loadAll();
    for (auto& e : entries) {
        if (e.status == CommitStatus::Committed) {
            committed[e.hash] = true;
        }
    }
    auto blocks = store->loadAllBlocks();
    if (!blocks.empty()) {
        cout << "[Recovery] Restored " << entries.size() << " committed entries, chain height "
                  << blocks.back().header.height << endl;
    }
}

void ConsensusManager::onLeaderFailover() {
    cout << "[Failover] Old leader lost. Switching to follower mode." << endl;
    leader = false;
    recoverCommitted();
}

ConsensusManager::~ConsensusManager() {
    stop();
}

#include "network/peerManager.hpp"
#include <iostream>

void PeerManager::broadcast(const Message& msg) {
    cout << "[PeerManager] Broadcast message type " << (int)msg.type << endl;
}

void PeerManager::sendTo(int peer_fd, const Message& msg) {
    cout << "[PeerManager] Send message type " << (int)msg.type
              << " to peer fd " << peer_fd << endl;
}

int PeerManager::peerCount() const {
    return 3;
}

void ConsensusManager::periodicSnapshot() {
    snapshot_mgr.createSnapshot();
    std::cout << "[Consensus] Snapshot created at height " << last_snapshot_height << std::endl;
}

void ConsensusManager::recoverFromSnapshot() {
    bool ok = snapshot_mgr.restoreFromSnapshot();
    if (ok) {
        std::cout << "[Consensus] Snapshot restored successfully." << std::endl;
    } else {
        std::cout << "[Consensus] No snapshot found to restore." << std::endl;
    }
}

#include "consensus/consensusManager.hpp"
#include <iostream>



void ConsensusManager::receiveAck(int peer_fd, const std::array<uint8_t,32>& hash) {
    std::lock_guard<std::mutex> lock(mtx);
    ack_count[hash] += 1;
    int total = ack_count[hash];
    if (total >= 2) {
        std::cout << "Block reached majority, committing." << std::endl;
        finality_mgr.markCommitted(hash, store->getLatestBlock().header.height);
        finalizeCommittedBlocks();
    }
}

void ConsensusManager::finalizeCommittedBlocks() {
    uint64_t currentHeight = store->getLatestBlock().header.height;
    if (currentHeight > finalized_height + 2) {
        uint64_t toFinalize = currentHeight - 2;
        finality_mgr.finalize(toFinalize);
        finalized_height = toFinalize;
        std::cout << "Finalized block height " << toFinalize << std::endl;
    }
}

bool ConsensusManager::detectDoubleCommit(const Block& b) {
    bool fork = finality_mgr.detectFork(b);
    if (fork) {
        std::cout << "Fork detected at height " << b.header.height << std::endl;
    }
    return fork;
}

void ConsensusManager::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    ack_count.clear();
    finalized_height = 0;
    finality_mgr.clear();
}


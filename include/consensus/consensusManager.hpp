#pragma once
#include <mutex>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <array>

#include "core/block.hpp"
#include "storage/blockStore.hpp"
#include "network/peerManager.hpp"
#include "storage/commitLog.hpp"
#include "storage/snapshotManager.hpp"
#include "consensus/finalityManager.hpp"  // ← fehlte in deiner Version

class ConsensusManager {
public:
    ConsensusManager(BlockStore* store, PeerManager* peers, bool isLeader);
    ~ConsensusManager();

    void start();
    void stop();
    void tick();
    void receiveAppend(const Block& block, int peer_fd);
    void receiveAck(int peer_fd, const std::array<uint8_t,32>& hash);
    void broadcastAppend(const Block& block);

    void finalizeCommittedBlocks();
    bool detectDoubleCommit(const Block& b);
    bool isCommitted(const std::array<uint8_t,32>& hash);
    void recoverCommitted();
    void onLeaderFailover();
    void periodicSnapshot();
    void recoverFromSnapshot();
    void clear();

private:
    BlockStore* store;
    PeerManager* peers;
    bool leader;
    std::mutex mtx;
    std::map<std::array<uint8_t,32>, int> ack_count;
    std::map<std::array<uint8_t,32>, bool> committed;
    uint64_t finalized_height = 0;
    uint64_t last_snapshot_height = 0;
    bool running = false;
    std::thread worker;

    CommitLog commit_log;
    SnapshotManager snapshot_mgr;
    FinalityManager finality_mgr;  // ← jetzt richtig deklariert
};


#pragma once
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include "core/block.hpp"
#include "core/transaction.hpp"
#include "storage/blockStore.hpp"
#include "light/lightClient.hpp"
#include "light/merkleProof.hpp"
#include "network/messages.hpp"

class FastSyncManager {
public:
    FastSyncManager(BlockStore* store, LightClient* client);
    void handleGetHeader(int peer_fd, uint64_t height);
    void handleHeader(const BlockHeader& h);
    void handleGetProofTx(int peer_fd, const std::array<uint8_t,32>& txHash);
    void handleProofTx(const MerkleProof& proof);
    void broadcastHeader(const BlockHeader& h);
    void attachPeer(int peer_fd);
    void clear();
private:
    BlockStore* store;
    LightClient* client;
    std::vector<int> peers;
    std::mutex mtx;
};


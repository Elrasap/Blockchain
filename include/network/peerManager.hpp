#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "core/block.hpp"
#include "core/transaction.hpp"
#include "network/fastSyncManager.hpp"
#include "network/messages.hpp"

class Mempool;
class SyncManager;

struct PeerInfo {
    std::string address;
    uint64_t height = 0;
    uint64_t lastSeen = 0;
};

class PeerManager {
public:
    PeerManager(int port, SyncManager* syncManager);
    explicit PeerManager(int port);
    ~PeerManager();

    void setSync(SyncManager* manager) { sync = manager; }
    void setMempool(Mempool* pool) { mempool = pool; }
    void attachFastSync(FastSyncManager* manager) { fastSync = manager; }

    void startServer();
    void stop();
    void connectToPeer(const std::string& host, int port);

    void broadcastTransaction(const Transaction& tx);
    void broadcast(const Message& msg);
    bool sendTo(int peerFd, const Message& msg);
    void broadcastBlock(const Block& block);
    void broadcastRaw(const std::vector<uint8_t>& data);

    int peerCount() const;
    int getPort() const { return listenPort_; }
    void addPeer(const std::string& address);
    std::vector<PeerInfo> getPeers() const;
    void updatePeerHeight(const std::string& address, uint64_t height);
    void markSeen(const std::string& address);
    bool isConnected(const std::string& host, int port) const;

private:
    void serverLoop();
    void startReader(int fd);
    void handleClient(int fd);
    void handleMessage(int fd, const Message& msg);
    void shutdownAllConnections();
    bool sendAll(int fd, const uint8_t* data, std::size_t size);

    int listenPort_;
    std::atomic<bool> running_{false};
    std::atomic<int> serverFd_{-1};
    std::thread serverThread_;

    std::map<int, int> sockets_;
    mutable std::mutex connectionMutex_;
    std::vector<std::thread> clientThreads_;
    mutable std::mutex threadMutex_;

    std::vector<PeerInfo> peers_;
    mutable std::mutex peerMutex_;

    Mempool* mempool = nullptr;
    SyncManager* sync = nullptr;
    FastSyncManager* fastSync = nullptr;
};

#pragma once
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "core/transaction.hpp"
#include "network/messages.hpp"
#include "network/fastSyncManager.hpp"

class Mempool;
class SyncManager;

struct PeerInfo {
    std::string address;
    uint64_t height = 0;
    uint64_t lastSeen = 0;
};

class PeerManager {
public:
    FastSyncManager* fastSync = nullptr;

    PeerManager(int port, SyncManager* sync);
    PeerManager(int port);

    void startServer();
    void stop();
    void connectToPeer(const std::string& host, int port);

    void broadcastTransaction(const Transaction& tx);
    void broadcast(const Message& msg);
    void sendTo(int peer_fd, const Message& msg);

    void setMempool(Mempool* mp);

    int peerCount() const;
    int getPort() const { return listen_port; }

    // Peer list management
    void addPeer(const std::string& addr);
    std::vector<PeerInfo> getPeers() const;
    void updatePeerHeight(const std::string& addr, uint64_t height);
    void markSeen(const std::string& addr);

    void attachFastSync(FastSyncManager* f) { fastSync = f; }

private:
    int listen_port;
    bool running = false;

    std::thread serverThread;

    // active socket connections
    std::map<int, int> sockets;
    std::mutex connMutex;

    // peers list
    std::vector<PeerInfo> peers;
    mutable std::mutex mtx;   // <-- THIS IS THE PEER LIST MUTEX

    Mempool* mempool = nullptr;
    SyncManager* sync = nullptr;

    void serverLoop();
    void handleClient(int client_fd);
    void handleMessage(int fd, const Message& msg);
    void shutdownAllConnections();
};


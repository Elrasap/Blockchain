#pragma once
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "core/transaction.hpp"
#include "core/block.hpp"          // <-- WICHTIG!
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
    void setSync(SyncManager* s) { sync = s; }

    FastSyncManager* fastSync = nullptr;

    PeerManager(int port, SyncManager* sync);
    PeerManager(int port);

    void startServer();
    void stop();
    void connectToPeer(const std::string& host, int port);

    void broadcastTransaction(const Transaction& tx);
    void broadcast(const Message& msg);
    void sendTo(int peer_fd, const Message& msg);

    void broadcastBlock(const Block& block);   // <===== NEU!

    void setMempool(Mempool* mp);

    int peerCount() const;
    int getPort() const { return listen_port; }

    void addPeer(const std::string& addr);
    std::vector<PeerInfo> getPeers() const;
    void updatePeerHeight(const std::string& addr, uint64_t height);
    void markSeen(const std::string& addr);

    void attachFastSync(FastSyncManager* f) { fastSync = f; }
    void broadcastRaw(const std::vector<uint8_t>& data);
    bool isConnected(const std::string& host, int port) const;
private:
    int listen_port;
    bool running = false;

    std::thread serverThread;

    std::map<int, int> sockets;
    mutable std::mutex connMutex;


    std::vector<PeerInfo> peers;
    mutable std::mutex mtx;

    Mempool* mempool = nullptr;
    SyncManager* sync = nullptr;

    void serverLoop();
    void handleClient(int client_fd);
    void handleMessage(int fd, const Message& msg);
    void shutdownAllConnections();
};


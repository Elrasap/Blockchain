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

class PeerManager {
public:
    FastSyncManager* fastSync;
    PeerManager(int port, SyncManager* sync);
    PeerManager(int port); // overload
    void startServer();
    void stop();
    void connectToPeer(const std::string& host, int port);
    void broadcastTransaction(const Transaction& tx);
    void setMempool(Mempool* mp);
    void broadcast(const Message& msg);
    void sendTo(int peer_fd, const Message& msg);
    int peerCount() const;
    int getPort() const { return listen_port; }
    void attachFastSync(FastSyncManager* f) { fastSync = f; }



private:
    int listen_port;
    bool running;
    std::thread serverThread;
    std::mutex connMutex;
    std::map<int, int> sockets;

    Mempool* mempool;
    SyncManager* sync;

    void serverLoop();
    void handleClient(int client_fd);
    void handleMessage(int fd, const Message& msg);
    void shutdownAllConnections();
};


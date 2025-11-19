#include "network/peerManager.hpp"
#include "network/syncManager.hpp"
#include "core/mempool.hpp"
#include "network/messages.hpp"
#include "core/transaction.hpp"
#include "core/block.hpp"

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <algorithm>
#include <ctime>

using namespace std;

SyncManager* global_sync = nullptr;

// --------------------------------------------------------
// Konstruktoren
// --------------------------------------------------------

PeerManager::PeerManager(int port, SyncManager* syncManager)
        : listen_port(port), sync(syncManager)
{
    cout << "[PeerManager] Created with sync at port " << port << endl;
}

PeerManager::PeerManager(int port)
        : listen_port(port)
{
    cout << "[PeerManager] Created on port " << port << endl;
}


// --------------------------------------------------------
// Peer list logic
// --------------------------------------------------------

void PeerManager::addPeer(const std::string& addr) {
    std::lock_guard<std::mutex> lock(mtx);

    for (auto& p : peers) {
        if (p.address == addr)
            return;
    }

    PeerInfo p;
    p.address = addr;
    p.lastSeen = time(nullptr);
    peers.push_back(p);
}

std::vector<PeerInfo> PeerManager::getPeers() const {
    std::lock_guard<std::mutex> lock(mtx);
    return peers;
}

void PeerManager::updatePeerHeight(const std::string& addr, uint64_t height) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& p : peers)
        if (p.address == addr)
            p.height = height;
}

void PeerManager::markSeen(const std::string& addr) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& p : peers)
        if (p.address == addr)
            p.lastSeen = time(nullptr);
}


// --------------------------------------------------------
// Peer count
// --------------------------------------------------------

int PeerManager::peerCount() const {
    std::lock_guard<std::mutex> lock(mtx);
    return peers.size();
}


// --------------------------------------------------------
// Networking: start/stop server
// --------------------------------------------------------

void PeerManager::startServer()
{
    running = true;
    serverThread = thread(&PeerManager::serverLoop, this);
}

void PeerManager::stop()
{
    shutdownAllConnections();
    running = false;

    {
        lock_guard<mutex> lock(connMutex);
        for (auto& [fd, _] : sockets)
            close(fd);
        sockets.clear();
    }

    if (serverThread.joinable())
        serverThread.join();
}


// --------------------------------------------------------
// Connect to peer (outgoing)
// --------------------------------------------------------

void PeerManager::connectToPeer(const std::string& host, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "[PeerManager] Failed to create socket\n";
        return;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        cerr << "[PeerManager] Invalid address: " << host << endl;
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "[PeerManager] Connection failed to "
             << host << ":" << port << endl;
        close(sock);
        return;
    }

    {
        lock_guard<mutex> lock(connMutex);
        sockets[sock] = port;
    }

    addPeer(host + ":" + to_string(port));

    cout << "[PeerManager] Connected to peer "
         << host << ":" << port << endl;
}


// --------------------------------------------------------
// Mempool attach
// --------------------------------------------------------

void PeerManager::setMempool(Mempool* mp)
{
    mempool = mp;
}


// --------------------------------------------------------
// broadcast transaction
// --------------------------------------------------------

void PeerManager::broadcastTransaction(const Transaction& tx)
{
    vector<uint8_t> serialized = tx.serialize();

    Message msg;
    msg.type = MessageType::TX_BROADCAST;
    msg.payload = serialized;

    auto encoded = encodeMessage(msg);

    lock_guard<mutex> lock(connMutex);

    for (auto& [fd, _] : sockets) {
        send(fd, encoded.data(), encoded.size(), 0);
    }
}


// --------------------------------------------------------
// broadcast generic message
// --------------------------------------------------------

void PeerManager::broadcast(const Message& msg)
{
    auto encoded = encodeMessage(msg);

    lock_guard<std::mutex> lock(connMutex);

    for (auto& [fd, _] : sockets) {
        send(fd, encoded.data(), encoded.size(), 0);
    }
}


// --------------------------------------------------------
// Server loop
// --------------------------------------------------------

void PeerManager::serverLoop()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "Failed to create server socket\n";
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Bind failed\n";
        close(server_fd);
        return;
    }

    listen(server_fd, 10);

    cout << "PeerManager listening on port " << listen_port << "\n";

    while (running) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);

        if (client_fd < 0) {
            if (!running) break;
            continue;
        }

        {
            lock_guard<mutex> lock(connMutex);
            sockets[client_fd] = ntohs(client_addr.sin_port);
        }

        thread(&PeerManager::handleClient, this, client_fd).detach();
    }

    close(server_fd);
}


// --------------------------------------------------------
// Handle incoming peer
// --------------------------------------------------------

void PeerManager::handleClient(int client_fd)
{
    char buffer[4096];

    while (running) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0)
            break;

        vector<uint8_t> data(buffer, buffer + bytes);
        Message msg = decodeMessage(data);

        if (msg.type == MessageType::TX_BROADCAST) {

            Transaction tx;
            tx.deserialize(msg.payload);

            if (mempool) {
                std::string err;
                if (!mempool->addTransactionValidated(tx, err)) {
                    std::cerr << "[PeerManager] Rejected incoming TX: "
                              << err << "\n";
                }
            }

            cout << "Received TX_BROADCAST on port "
                 << listen_port << endl;
        }
        else if (msg.type == MessageType::INV ||
                 msg.type == MessageType::GETBLOCK ||
                 msg.type == MessageType::BLOCK) {

            handleMessage(client_fd, msg);
        }
    }

    close(client_fd);

    {
        lock_guard<mutex> lock(connMutex);
        sockets.erase(client_fd);
    }
}


// --------------------------------------------------------
// Sync messages
// --------------------------------------------------------

void PeerManager::handleMessage(int fd, const Message& msg)
{
    if (msg.type == MessageType::HEADER) {
        BlockHeader h = decodeHeader(msg.payload);
        if (fastSync) fastSync->handleHeader(h);
    }

    if (msg.type == MessageType::INV) {
        array<uint8_t, 32> h{};
        memcpy(h.data(), msg.payload.data(), 32);
        global_sync->handleInv(h);
    }
    else if (msg.type == MessageType::GETBLOCK) {
        array<uint8_t, 32> h{};
        memcpy(h.data(), msg.payload.data(), 32);
        global_sync->handleGetBlock(h, fd);
    }
    else if (msg.type == MessageType::BLOCK) {
        Block b;
        b.deserialize(msg.payload);
        global_sync->handleBlock(b);
    }
}


// --------------------------------------------------------
// Shutdown
// --------------------------------------------------------

void PeerManager::shutdownAllConnections()
{
    lock_guard<mutex> lock(connMutex);

    for (auto& [fd, _] : sockets)
        shutdown(fd, SHUT_RDWR);
}


// --------------------------------------------------------
// BLOCK BROADCAST
// --------------------------------------------------------

void PeerManager::broadcastBlock(const Block& block)
{
    std::vector<uint8_t> bytes = block.serialize();

    Message msg;
    msg.type = MessageType::BLOCK;
    msg.payload = bytes;

    std::vector<uint8_t> encoded = encodeMessage(msg);

    std::lock_guard<std::mutex> lock(connMutex);

    for (auto& [fd, _] : sockets) {
        send(fd, encoded.data(), encoded.size(), 0);
    }

    std::cout << "[PeerManager] broadcastBlock height="
              << block.header.height
              << " to " << sockets.size()
              << " peers\n";
}

void PeerManager::broadcastRaw(const std::vector<uint8_t>& data)
{
    std::lock_guard<std::mutex> lock(connMutex);
    for (auto& [fd, _] : sockets)
        send(fd, data.data(), data.size(), 0);
}
bool PeerManager::isConnected(const std::string& host, int port) const
{
    std::lock_guard<std::mutex> lock(connMutex);

    std::string full = host + ":" + std::to_string(port);

    for (auto& p : peers)
        if (p.address == full)
            return true;

    return false;
}


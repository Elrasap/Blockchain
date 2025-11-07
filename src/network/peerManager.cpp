#include "network/peerManager.hpp"
#include "network/messages.hpp"
#include "core/transaction.hpp"
#include "network/syncManager.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include "network/peerManager.hpp"
#include "network/syncManager.hpp"
#include "core/mempool.hpp"

using namespace std;

SyncManager* global_sync = nullptr;

#include "network/peerManager.hpp"
#include <iostream>

#include "network/peerManager.hpp"
#include <iostream>

PeerManager::PeerManager(int port) : listen_port(port) {
    std::cout << "[PeerManager] Created on port " << port << std::endl;
}

void PeerManager::startServer() {
    running = true;
    serverThread = thread(&PeerManager::serverLoop, this);
}

void PeerManager::stop() {
    shutdownAllConnections();
    running = false;
    {
        lock_guard<mutex> lock(connMutex);
        for (auto& [fd, _] : sockets) close(fd);
        sockets.clear();
    }
    if (serverThread.joinable()) serverThread.join();
}

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

void PeerManager::connectToPeer(const std::string& host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[PeerManager] Failed to create socket\n";
        return;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "[PeerManager] Invalid address: " << host << std::endl;
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[PeerManager] Connection failed to " << host << ":" << port << std::endl;
        close(sock);
        return;
    }

    std::lock_guard<std::mutex> lock(connMutex);
    sockets[port] = sock;
    std::cout << "[PeerManager] Connected to peer " << host << ":" << port << std::endl;
}

void PeerManager::broadcastTransaction(const Transaction& tx) {
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

void PeerManager::serverLoop() {
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

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Bind failed\n";
        close(server_fd);
        cout << "Server thread on port " << listen_port << " stopped." << endl;

        return;
    }

    listen(server_fd, 10);
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
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

void PeerManager::handleClient(int client_fd) {
    char buffer[4096];
    while (running) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        vector<uint8_t> data(buffer, buffer + bytes);
        Message msg = decodeMessage(data);

        if (msg.type == MessageType::TX_BROADCAST) {
            Transaction tx;
            tx.deserialize(msg.payload);
            if (mempool) mempool->addTransaction(tx);
            cout << "Received TX_BROADCAST on port " << listen_port << endl;
        } else if (msg.type == MessageType::INV ||
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

void PeerManager::handleMessage(int fd, const Message& msg) {
    if (msg.type == MessageType::HEADER) {
        BlockHeader h = decodeHeader(msg.payload);
        if (fastSync) fastSync->handleHeader(h);

    }

    if (msg.type == MessageType::INV) {
        array<uint8_t, 32> h{};
        memcpy(h.data(), msg.payload.data(), 32);
        global_sync->handleInv(h);
    } else if (msg.type == MessageType::GETBLOCK) {
        array<uint8_t, 32> h{};
        memcpy(h.data(), msg.payload.data(), 32);
        global_sync->handleGetBlock(h, fd);
    } else if (msg.type == MessageType::BLOCK) {
        Block b;
        b.deserialize(msg.payload);
        global_sync->handleBlock(b);
    }
}

void PeerManager::setMempool(Mempool* mp) {
    mempool = mp;
}

void PeerManager::shutdownAllConnections() {
    lock_guard<mutex> lock(connMutex);
    for (auto& [fd, _] : sockets) {
        shutdown(fd, SHUT_RDWR);
    }
}

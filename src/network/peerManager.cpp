#include "network/peerManager.hpp"

#include "core/mempool.hpp"
#include "network/syncManager.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <ctime>

PeerManager::PeerManager(int port, SyncManager* syncManager)
    : listenPort_(port), sync(syncManager)
{
}

PeerManager::PeerManager(int port)
    : listenPort_(port)
{
}

PeerManager::~PeerManager()
{
    stop();
}

void PeerManager::addPeer(const std::string& address)
{
    std::lock_guard<std::mutex> lock(peerMutex_);
    for (auto& peer : peers_)
        if (peer.address == address)
            return;
    peers_.push_back({address, 0, static_cast<uint64_t>(std::time(nullptr))});
}

std::vector<PeerInfo> PeerManager::getPeers() const
{
    std::lock_guard<std::mutex> lock(peerMutex_);
    return peers_;
}

void PeerManager::updatePeerHeight(const std::string& address, uint64_t height)
{
    std::lock_guard<std::mutex> lock(peerMutex_);
    for (auto& peer : peers_)
        if (peer.address == address)
            peer.height = height;
}

void PeerManager::markSeen(const std::string& address)
{
    std::lock_guard<std::mutex> lock(peerMutex_);
    for (auto& peer : peers_)
        if (peer.address == address)
            peer.lastSeen = static_cast<uint64_t>(std::time(nullptr));
}

int PeerManager::peerCount() const
{
    std::lock_guard<std::mutex> lock(connectionMutex_);
    return static_cast<int>(sockets_.size());
}

void PeerManager::startServer()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
        return;
    serverThread_ = std::thread(&PeerManager::serverLoop, this);
}

void PeerManager::stop()
{
    running_ = false;

    const int server = serverFd_.exchange(-1);
    if (server >= 0) {
        ::shutdown(server, SHUT_RDWR);
        ::close(server);
    }
    shutdownAllConnections();

    if (serverThread_.joinable())
        serverThread_.join();

    std::lock_guard<std::mutex> lock(threadMutex_);
    for (auto& thread : clientThreads_)
        if (thread.joinable())
            thread.join();
    clientThreads_.clear();
}

void PeerManager::connectToPeer(const std::string& host, int port)
{
    const int socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        std::cerr << "[PeerManager] Failed to create socket\n";
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1 ||
        ::connect(socketFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "[PeerManager] Connection failed to "
                  << host << ':' << port << "\n";
        ::close(socketFd);
        return;
    }

    timeval timeout{3, 0};
    ::setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        sockets_[socketFd] = port;
    }
    addPeer(host + ":" + std::to_string(port));
    startReader(socketFd);
    if (sync) sync->sendTip(socketFd);
}

bool PeerManager::sendAll(int fd, const uint8_t* data, std::size_t size)
{
    std::size_t sent = 0;
    while (sent < size && running_) {
        const ssize_t result = ::send(fd, data + sent, size - sent, MSG_NOSIGNAL);
        if (result > 0) {
            sent += static_cast<std::size_t>(result);
            continue;
        }
        if (result < 0 && errno == EINTR)
            continue;
        return false;
    }
    return sent == size;
}

bool PeerManager::sendTo(int peerFd, const Message& msg)
{
    try {
        const auto encoded = encodeMessage(msg);
        return sendAll(peerFd, encoded.data(), encoded.size());
    } catch (const std::exception& ex) {
        std::cerr << "[PeerManager] Could not encode message: "
                  << ex.what() << "\n";
        return false;
    }
}

void PeerManager::broadcast(const Message& msg)
{
    std::vector<int> sockets;
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        for (const auto& [fd, ignored] : sockets_) {
            (void)ignored;
            sockets.push_back(fd);
        }
    }
    for (int fd : sockets)
        sendTo(fd, msg);
}

void PeerManager::broadcastTransaction(const Transaction& tx)
{
    broadcast({MessageType::TX_BROADCAST, tx.serialize()});
}

void PeerManager::broadcastBlock(const Block& block)
{
    broadcast({MessageType::BLOCK, block.serialize()});
}

void PeerManager::broadcastRaw(const std::vector<uint8_t>& data)
{
    std::vector<int> sockets;
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        for (const auto& [fd, ignored] : sockets_) {
            (void)ignored;
            sockets.push_back(fd);
        }
    }
    for (int fd : sockets)
        sendAll(fd, data.data(), data.size());
}

void PeerManager::serverLoop()
{
    const int server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        running_ = false;
        return;
    }
    serverFd_ = server;

    int reuse = 1;
    ::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(listenPort_);
    address.sin_addr.s_addr = INADDR_ANY;

    if (::bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
        ::listen(server, 16) < 0) {
        std::cerr << "[PeerManager] Could not listen on port "
                  << listenPort_ << "\n";
        running_ = false;
        ::close(server);
        serverFd_ = -1;
        return;
    }

    while (running_) {
        sockaddr_in clientAddress{};
        socklen_t length = sizeof(clientAddress);
        const int client = ::accept(server,
            reinterpret_cast<sockaddr*>(&clientAddress), &length);
        if (client < 0) {
            if (running_ && errno == EINTR) continue;
            break;
        }

        timeval timeout{3, 0};
        ::setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        {
            std::lock_guard<std::mutex> lock(connectionMutex_);
            sockets_[client] = ntohs(clientAddress.sin_port);
        }
        startReader(client);
        if (sync) sync->sendTip(client);
    }

    if (serverFd_.exchange(-1) == server)
        ::close(server);
}

void PeerManager::startReader(int fd)
{
    std::lock_guard<std::mutex> lock(threadMutex_);
    clientThreads_.emplace_back(&PeerManager::handleClient, this, fd);
}

void PeerManager::handleClient(int clientFd)
{
    std::vector<uint8_t> receiveBuffer;
    receiveBuffer.reserve(8192);
    std::array<uint8_t, 4096> chunk{};

    bool validConnection = true;
    while (running_ && validConnection) {
        const ssize_t count = ::recv(clientFd, chunk.data(), chunk.size(), 0);
        if (count == 0) break;
        if (count < 0) {
            if (errno == EINTR) continue;
            break;
        }

        receiveBuffer.insert(receiveBuffer.end(), chunk.begin(),
                             chunk.begin() + count);
        if (receiveBuffer.size() >
            MAX_MESSAGE_PAYLOAD + MESSAGE_HEADER_SIZE + chunk.size()) {
            std::cerr << "[PeerManager] Peer exceeded receive limit\n";
            break;
        }

        while (!receiveBuffer.empty()) {
            Message message;
            std::size_t consumed = 0;
            std::string error;
            const auto result = tryDecodeMessageFrame(receiveBuffer, message,
                                                      consumed, error);
            if (result == FrameDecodeResult::NeedMoreData)
                break;
            if (result == FrameDecodeResult::Invalid) {
                std::cerr << "[PeerManager] Invalid frame: " << error << "\n";
                validConnection = false;
                break;
            }

            try {
                handleMessage(clientFd, message);
            } catch (const std::exception& ex) {
                std::cerr << "[PeerManager] Rejected message: "
                          << ex.what() << "\n";
            }
            receiveBuffer.erase(receiveBuffer.begin(),
                                receiveBuffer.begin() + static_cast<std::ptrdiff_t>(consumed));
        }
    }

    ::shutdown(clientFd, SHUT_RDWR);
    ::close(clientFd);
    std::lock_guard<std::mutex> lock(connectionMutex_);
    sockets_.erase(clientFd);
}

void PeerManager::handleMessage(int fd, const Message& msg)
{
    if (msg.type == MessageType::TX_BROADCAST) {
        if (!mempool) return;
        Transaction tx;
        std::string error;
        if (!Transaction::deserialize(msg.payload, tx, error) ||
            !mempool->addTransactionValidated(tx, error)) {
            std::cerr << "[PeerManager] Rejected transaction: "
                      << error << "\n";
        }
        return;
    }

    if (msg.type == MessageType::HEADER) {
        BlockHeader header = decodeHeader(msg.payload);
        if (fastSync) fastSync->handleHeader(header);
        return;
    }

    if (!sync) return;

    if (msg.type == MessageType::INV || msg.type == MessageType::GETBLOCK) {
        if (msg.payload.size() != 32)
            throw std::runtime_error("block hash message must contain 32 bytes");
        std::array<uint8_t, 32> hash{};
        std::copy(msg.payload.begin(), msg.payload.end(), hash.begin());
        if (msg.type == MessageType::INV)
            sync->handleInv(hash);
        else
            sync->handleGetBlock(hash, fd);
        return;
    }

    if (msg.type == MessageType::BLOCK) {
        Block block;
        std::string error;
        if (!Block::deserialize(msg.payload, block, error))
            throw std::runtime_error(error);
        sync->handleBlock(block);
    }
}

void PeerManager::shutdownAllConnections()
{
    std::lock_guard<std::mutex> lock(connectionMutex_);
    for (const auto& [fd, ignored] : sockets_) {
        (void)ignored;
        ::shutdown(fd, SHUT_RDWR);
    }
}

bool PeerManager::isConnected(const std::string& host, int port) const
{
    const std::string expected = host + ":" + std::to_string(port);
    std::lock_guard<std::mutex> lock(peerMutex_);
    return std::any_of(peers_.begin(), peers_.end(),
        [&](const PeerInfo& peer) { return peer.address == expected; });
}

#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>

class PeerManager;

class PeerDiscovery {
public:
    PeerDiscovery(PeerManager& mgr, const std::string& path);

    void start();
    void stop();

    // Nur einmal beim Start ausgeführt:
    void loadPeersFile();

    // Direkt Peers speichern (optional)
    void savePeersFile();

private:
    void loop();

private:
    PeerManager& peers_;
    std::string path_;

    std::thread worker_;
    std::atomic<bool> running{false};
};


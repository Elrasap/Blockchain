#pragma once
#include <string>
#include <chrono>

class ClusterNode {
public:
    ClusterNode(int id, const std::string& version);
    void tick();
    bool isAlive() const;
    void fail();
    void recover();
    void upgrade(const std::string& newVersion);
    std::string status() const;
private:
    int nodeId;
    bool alive;
    std::string version;
    std::chrono::steady_clock::time_point lastHeartbeat;
};


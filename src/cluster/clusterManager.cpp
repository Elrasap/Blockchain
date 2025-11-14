#include "cluster/cluster_manager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

ClusterManager::ClusterManager(int size, const std::string& version) {
    for (int i = 0; i < size; ++i)
        nodes.emplace_back(i, version);
}

void ClusterManager::printStatus() const {
    for (const auto& n : nodes)
        std::cout << n.status() << "\n";
    std::cout << "-----------------------\n";
}

void ClusterManager::simulateTicks(int cycles) {
    for (int i = 0; i < cycles; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        for (auto& n : nodes) n.tick();
        if (i % 10 == 5 && !nodes.empty()) {
            nodes[0].fail();
            std::cout << "[Cluster] Node 0 failed.\n";
        }
        if (i % 10 == 8 && !nodes.empty()) {
            nodes[0].recover();
            std::cout << "[Cluster] Node 0 recovered.\n";
        }
        printStatus();
    }
}

void ClusterManager::rollingUpgrade(const std::string& newVersion) {
    std::cout << "[Cluster] Starting rolling upgrade to " << newVersion << "\n";
    for (auto& n : nodes) {
        std::cout << "[Cluster] Upgrading node...\n";
        n.fail();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        n.upgrade(newVersion);
        n.recover();
        printStatus();
    }
    std::cout << "[Cluster] Upgrade complete.\n";
}


#include "runtime/nodeHealthMonitor.hpp"
#include <chrono>

NodeHealthMonitor::NodeHealthMonitor()
    : lastCommit(std::chrono::steady_clock::now()), consecutiveFails(0) {}

void NodeHealthMonitor::recordBlockCommit() {
    lastCommit = std::chrono::steady_clock::now();
    consecutiveFails = 0;
}

bool NodeHealthMonitor::isHealthy() const {
    using namespace std::chrono;
    auto diff = duration_cast<seconds>(steady_clock::now() - lastCommit).count();
    return diff < 30;
}

bool NodeHealthMonitor::shouldRestart() const {
    return consecutiveFails > 3;
}

void NodeHealthMonitor::tick() {
    if (!isHealthy())
        consecutiveFails++;
}


#pragma once
#include <string>
#include <thread>
#include <atomic>
#include "core/logger.hpp"
#include "obs/health_checker.hpp"
#include "node/lifecycle_manager.hpp"
#include "node/recovery_controller.hpp"

enum class NodeState { Ready, Draining, Restarting, Rejoining, Healthy };

class NodeInstance {
public:
    NodeInstance(std::string name, int id);
    void start();
    void stop();
    void restart();
    void simulateUpgrade();
    NodeState getState() const;
    std::string getName() const;
private:
    std::string name;
    int id;
    std::atomic<NodeState> state;
    void run();
};


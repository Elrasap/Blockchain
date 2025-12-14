#include "node/nodeInstance.hpp"
#include <chrono>

NodeInstance::NodeInstance(std::string n, int i)
    : name(std::move(n)), id(i), state(NodeState::Ready) {}

void NodeInstance::start() {
    Logger::instance().log(LogLevel::INFO, name + " starting");
    state = NodeState::Ready;
}

void NodeInstance::run() {
    Logger::instance().log(LogLevel::INFO, name + " running");
    HealthChecker::instance().setBlockHeight(id * 10);
    HealthChecker::instance().setPeerCount(3);
}

void NodeInstance::stop() {
    Logger::instance().log(LogLevel::WARN, name + " draining");
    state = NodeState::Draining;
}

void NodeInstance::restart() {
    Logger::instance().log(LogLevel::INFO, name + " restarting");
    state = NodeState::Restarting;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    Logger::instance().log(LogLevel::INFO, name + " rejoining");
    state = NodeState::Rejoining;
}

void NodeInstance::simulateUpgrade() {
    stop();
    restart();
    Logger::instance().log(LogLevel::INFO, name + " healthy");
    state = NodeState::Healthy;
}

NodeState NodeInstance::getState() const {
    return state.load();
}

std::string NodeInstance::getName() const {
    return name;
}


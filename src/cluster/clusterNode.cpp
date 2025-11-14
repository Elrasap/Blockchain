#include "cluster/cluster_node.hpp"
#include <chrono>
#include <sstream>

ClusterNode::ClusterNode(int id, const std::string& v)
    : nodeId(id), alive(true), version(v),
      lastHeartbeat(std::chrono::steady_clock::now()) {}

void ClusterNode::tick() {
    if (alive)
        lastHeartbeat = std::chrono::steady_clock::now();
}

bool ClusterNode::isAlive() const {
    using namespace std::chrono;
    return alive && duration_cast<seconds>(steady_clock::now() - lastHeartbeat).count() < 5;
}

void ClusterNode::fail() { alive = false; }

void ClusterNode::recover() {
    alive = true;
    lastHeartbeat = std::chrono::steady_clock::now();
}

void ClusterNode::upgrade(const std::string& newVersion) { version = newVersion; }

std::string ClusterNode::status() const {
    std::ostringstream ss;
    ss << "Node " << nodeId << " [" << version << "] "
       << (alive ? "ALIVE" : "DEAD");
    return ss.str();
}


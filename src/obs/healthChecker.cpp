#include "obs/healthChecker.hpp"
#include <sstream>

HealthChecker& HealthChecker::instance() {
    static HealthChecker inst;
    return inst;
}

void HealthChecker::setBlockHeight(uint64_t h) {
    std::lock_guard<std::mutex> lock(mtx);
    height = h;
}

void HealthChecker::setPeerCount(size_t c) {
    std::lock_guard<std::mutex> lock(mtx);
    peers = c;
}

void HealthChecker::markUnhealthy(const std::string& r) {
    std::lock_guard<std::mutex> lock(mtx);
    healthy = false;
    reason = r;
}

void HealthChecker::markHealthy() {
    std::lock_guard<std::mutex> lock(mtx);
    healthy = true;
    reason.clear();
}

std::string HealthChecker::statusJson() {
    std::lock_guard<std::mutex> lock(mtx);
    std::ostringstream ss;
    ss << "{\"healthy\":" << (healthy ? "true" : "false")
       << ",\"height\":" << height
       << ",\"peers\":" << peers;
    if (!healthy && !reason.empty())
        ss << ",\"reason\":\"" << reason << "\"";
    ss << "}";
    return ss.str();
}


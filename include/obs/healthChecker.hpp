#pragma once
#include <string>
#include <mutex>

class HealthChecker {
public:
    static HealthChecker& instance();
    void setBlockHeight(uint64_t h);
    void setPeerCount(size_t c);
    void markUnhealthy(const std::string& reason);
    void markHealthy();
    std::string statusJson();
private:
    HealthChecker() = default;
    uint64_t height = 0;
    size_t peers = 0;
    bool healthy = true;
    std::string reason;
    std::mutex mtx;
};


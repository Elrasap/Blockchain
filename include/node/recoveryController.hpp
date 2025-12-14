#pragma once
#include <string>
#include <atomic>
#include "core/logger.hpp"
#include "obs/metrics.hpp"

class RecoveryController {
public:
    static RecoveryController& instance();
    void simulateCrash(const std::string& component);
    void recoverComponent(const std::string& component);
    bool isRecovering() const;
private:
    RecoveryController() = default;
    std::atomic<bool> recovering{false};
};


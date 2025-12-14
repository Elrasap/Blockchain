#include "node/recoveryController.hpp"

RecoveryController& RecoveryController::instance() {
    static RecoveryController inst;
    return inst;
}

void RecoveryController::simulateCrash(const std::string& component) {
    Logger::instance().log(LogLevel::ERROR, component + " crashed");
    recovering = true;
    Metrics::instance().incCounter("crashes_total");
}

void RecoveryController::recoverComponent(const std::string& component) {
    Logger::instance().log(LogLevel::INFO, "restarting " + component);
    recovering = false;
    Metrics::instance().incCounter("recoveries_total");
}

bool RecoveryController::isRecovering() const {
    return recovering;
}


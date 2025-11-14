#include "alerts/alert_manager.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

AlertManager::AlertManager(EventCollector& c) : collector(c) {}

void AlertManager::addRule(const AlertRule& r) { rules.push_back(r); }

void AlertManager::sendAlert(const std::string& msg, const std::string& severity) {
    std::ostringstream ss;
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ss << "[" << std::ctime(&now) << "] "
       << "ALERT (" << severity << "): " << msg << "\n";
    std::cout << ss.str();
}

void AlertManager::evaluate() {
    auto events = collector.getRecentEvents();
    for (const auto& e : events) {
        for (const auto& r : rules) {
            if (e.type == "METRIC" && e.message == r.metric && e.value > r.threshold) {
                sendAlert("Metric " + r.metric + " exceeded threshold (" +
                          std::to_string(e.value) + " > " + std::to_string(r.threshold) + ")",
                          r.severity);
            }
        }
    }
}


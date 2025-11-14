#pragma once
#include <vector>
#include <string>
#include "alerts/alert_rule.hpp"
#include "alerts/event_collector.hpp"

class AlertManager {
public:
    explicit AlertManager(EventCollector& collector);
    void addRule(const AlertRule& r);
    void evaluate();
private:
    EventCollector& collector;
    std::vector<AlertRule> rules;
    void sendAlert(const std::string& msg, const std::string& severity);
};


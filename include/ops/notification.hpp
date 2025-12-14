#pragma once
#include <string>
#include "ops/reliabilityGuard.hpp"

enum class AlertLevel { INFO, WARNING, CRITICAL };

class Notifier {
public:
    Notifier(const std::string& webhook, const std::string& logfile);
    void sendAlert(const ReliabilityStatus& status, AlertLevel level);
private:
    std::string webhookUrl;
    std::string logFile;
    void sendWebhook(const std::string& message);
    void writeLog(const std::string& message);
};


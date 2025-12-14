#pragma once
#include <string>
#include <thread>
#include <atomic>

class ExportAgent {
public:
    ExportAgent(const std::string& endpoint, int intervalSec);
    ~ExportAgent();
    void start();
    void stop();
private:
    std::string endpointUrl;
    int interval;
    std::atomic<bool> running;
    std::thread worker;
    void run();
    std::string collectMetricsJson();
    void sendJson(const std::string& payload);
};


#pragma once
#include <thread>
#include <atomic>

class MetricsServer {
public:
    explicit MetricsServer(int port);
    ~MetricsServer();

    void start();
    void stop();

private:
    int port;
    std::atomic<bool> running;
    std::thread serverThread;
};


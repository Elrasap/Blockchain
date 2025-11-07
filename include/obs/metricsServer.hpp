#pragma once
#include <thread>
#include <atomic>
#include <string>

class MetricsServer {
public:
    explicit MetricsServer(int port);
    void start();
    void stop();
private:
    int port;
    std::thread server_thread;
    std::atomic<bool> running;
    void run();
};


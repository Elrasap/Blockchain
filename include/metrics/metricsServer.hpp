#pragma once
#include <thirdparty/httplib.h>
#include <thread>
#include <atomic>

class MetricsServer {
private:
    int port;
    std::atomic<bool> running;
    std::thread server_thread;
    httplib::Server svr;

public:
    MetricsServer(int port);
    ~MetricsServer();

    bool start();   // now returns success/failure
    void stop();
};


#pragma once
#include <thirdparty/httplib.h>
#include <thread>
#include <atomic>

class MetricsServer {
private:
    int port;
    std::atomic<bool> running;
    std::thread server_thread;
    httplib::Server svr;   // eigener Server (Port 9100)

public:
    MetricsServer(int port);
    ~MetricsServer();

    bool start();   // starts standalone server
    void stop();

    // ➜ NEU: attach an existing server (/metrics + /health)
    void attach(httplib::Server& http);
};


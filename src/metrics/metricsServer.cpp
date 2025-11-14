#include "metrics/metricsServer.hpp"
#include "obs/metrics.hpp"
#include "obs/healthChecker.hpp"
#include <iostream>

MetricsServer::MetricsServer(int p)
    : port(p), running(false) {}

MetricsServer::~MetricsServer() {
    stop();
}

void MetricsServer::start() {
    if (running) return;
    running = true;

    // HTTP Endpoints
    svr.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        std::string body = Metrics::instance().renderPrometheus();
        res.set_content(body, "text/plain");
    });

    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        std::string json = HealthChecker::instance().statusJson();
        res.set_content(json, "application/json");
    });

    std::cout << "[MetricsServer] Running on http://localhost:" << port << "\n";

    server_thread = std::thread([this]() {
        svr.listen("0.0.0.0", port);
        running = false;
    });
}

void MetricsServer::stop() {
    if (!running) return;
    running = false;

    svr.stop();  // Wichtig

    if (server_thread.joinable())
        server_thread.join();
}


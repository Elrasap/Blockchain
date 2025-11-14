#include "metrics/metricsServer.hpp"
#include "obs/metrics.hpp"
#include "obs/healthChecker.hpp"

#include <iostream>
#include <exception>

MetricsServer::MetricsServer(int p)
    : port(p), running(false) {}

MetricsServer::~MetricsServer() {
    stop();
}

bool MetricsServer::start() {
    if (running.load()) {
        std::cerr << "[MetricsServer] Already running, ignoring second start.\n";
        return false;   // second start prevented
    }

    running = true;

    // SAFE ENDPOINT: /metrics
    svr.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        try {
            auto body = Metrics::instance().renderPrometheus();
            if (body.empty()) body = "# no metrics\n";
            res.set_content(body, "text/plain");
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(std::string("error: ") + e.what(), "text/plain");
        }
    });

    // SAFE ENDPOINT: /health
    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        try {
            auto json = HealthChecker::instance().statusJson();
            if (json.empty()) json = R"({"healthy":false,"reason":"empty"})";
            res.set_content(json, "application/json");
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
        }
    });

    std::cout << "[MetricsServer] Starting http://localhost:" << port << "\n";

    server_thread = std::thread([this]() {
        try {
            bool ok = svr.listen("0.0.0.0", port);
            if (!ok) {
                std::cerr << "[MetricsServer] Port " << port << " already in use!\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MetricsServer] Exception inside httplib: "
                      << e.what() << "\n";
        }
        running = false;
    });

    return true;
}

void MetricsServer::stop() {
    if (!running.load())
        return;

    running = false;

    svr.stop(); // stops listen()

    if (server_thread.joinable())
        server_thread.join();

    std::cout << "[MetricsServer] Stopped.\n";
}


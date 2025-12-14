#include "metrics/metricsServer.hpp"
#include "obs/metrics.hpp"
#include "obs/healthChecker.hpp"

#include <iostream>
#include <exception>

// ------------------------------------------------------------
// Constructor / Destructor
// ------------------------------------------------------------
MetricsServer::MetricsServer(int p)
    : port(p), running(false) {}

MetricsServer::~MetricsServer() {
    stop();
}

// ------------------------------------------------------------
// Standalone server (port 9100 for Prometheus)
// ------------------------------------------------------------
bool MetricsServer::start() {
    if (running.load()) {
        std::cerr << "[MetricsServer] Already running.\n";
        return false;
    }

    running = true;

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

    svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        try {
            auto json = HealthChecker::instance().statusJson();
            if (json.empty()) json = R"({"healthy":false,"reason":"empty"})";
            res.set_content(json, "application/json");
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content(std::string("{\"error\":\"") +
                            e.what() + "\"}", "application/json");
        }
    });

    std::cout << "[MetricsServer] Starting standalone http://0.0.0.0:"
              << port << "\n";

    server_thread = std::thread([this]() {
        try {
            bool ok = svr.listen("0.0.0.0", port);
            if (!ok)
                std::cerr << "[MetricsServer] Port busy: " << port << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << "[MetricsServer] Exception: " << e.what() << "\n";
        }
        running = false;
    });

    return true;
}

// ------------------------------------------------------------
// Stop standalone server
// ------------------------------------------------------------
void MetricsServer::stop() {
    if (!running.load())
        return;

    running = false;
    svr.stop();

    if (server_thread.joinable())
        server_thread.join();

    std::cout << "[MetricsServer] Stopped.\n";
}

// ------------------------------------------------------------
// NEW: Attach to main HTTP server
// ------------------------------------------------------------
void MetricsServer::attach(httplib::Server& http)
{
    std::cout << "[MetricsServer] Attaching /metrics + /health to main HTTP server\n";

    http.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        auto body = Metrics::instance().renderPrometheus();
        if (body.empty()) body = "# no metrics\n";
        res.set_content(body, "text/plain");
    });

    http.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        auto json = HealthChecker::instance().statusJson();
        if (json.empty()) json = R"({"healthy":false})";
        res.set_content(json, "application/json");
    });
}


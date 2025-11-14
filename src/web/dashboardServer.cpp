#include "web/dashboardServer.hpp"
#include <thirdparty/httplib.h>
#include <iostream>
#include <fstream>
#include <filesystem>   // <-- WICHTIG!!!

DashboardServer::DashboardServer(int port,
                                 const std::string& reportsDir,
                                 const std::string& binaryPath)
    : port(port), reportsDir(reportsDir), binaryPath(binaryPath) {}

void DashboardServer::start() {
    using namespace httplib;
    namespace fs = std::filesystem;

    Server svr;

    svr.Get("/", [&](const Request&, Response& res) {
        std::string html;

        html += "<html><body>";
        html += "<h1>Blockchain Dashboard</h1>";
        html += "<ul>";
        html += "<li><a href=\"/reports\">Reports</a></li>";
        html += "<li><a href=\"/binary\">Download Binary</a></li>";
        html += "<li><a href=\"/metrics\">Metrics</a></li>";
        html += "</ul>";
        html += "</body></html>";

        res.set_content(html, "text/html");
    });

    // ----- Reports -----
    svr.Get("/reports", [&](const Request&, Response& res) {
        std::string html = "<html><body><h2>Reports</h2><ul>";

        try {
            for (const auto& entry : fs::directory_iterator(reportsDir)) {
                html += "<li>" + entry.path().string() + "</li>";
            }
        } catch (...) {
            html += "<li>No reports found</li>";
        }

        html += "</ul></body></html>";
        res.set_content(html, "text/html");
    });

    // ----- Binary Download -----
    svr.Get("/binary", [&](const Request&, Response& res) {
        std::ifstream in(binaryPath, std::ios::binary);
        if (!in.is_open()) {
            res.status = 404;
            res.set_content("binary not found", "text/plain");
            return;
        }

        std::string buf(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>()
        );

        res.set_content(buf, "application/octet-stream");
    });

    // ----- INTERNAL METRICS REDIRECT -----
    svr.Get("/metrics", [&](const Request&, Response& res) {
        res.set_redirect("http://localhost:9100/metrics");
    });

    std::cout << "[DashboardServer] Listening on 0.0.0.0:" << port << "\n";

    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "[DashboardServer] ERROR: Could not bind port " << port << "\n";
    }
}


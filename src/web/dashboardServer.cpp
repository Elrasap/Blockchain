#include "web/dashboardServer.hpp"
#include "web/chainApi.hpp"
#include "web/dndApi.hpp"

#include "thirdparty/httplib.h"
#include <filesystem>
#include <iostream>

DashboardServer::DashboardServer(int port,
                                 const std::string& reportsDir,
                                 const std::string& binaryPath)
    : port(port), reportsDir(reportsDir), binaryPath(binaryPath)
{
}

/*
    IMPORTANT:
    DashboardServer::start() must NOT run its own HTTP server.
    The main.cpp owns the httplib::Server instance.
    DashboardServer only attaches endpoints to the existing server.
*/
void DashboardServer::start()
{
    std::cerr << "[DashboardServer] WARNING: start() was called but is disabled. "
                 "Use attach() instead.\n";
}

// ---------------------------------------------------------
// Attach dashboard routes to the main HTTP server
// ---------------------------------------------------------
void DashboardServer::attach(httplib::Server& http)
{
    // Root page for browser
    http.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        std::string html =
            "<html><body style='font-family:sans-serif; background:#222; color:#eee;'>"
            "<h1>Blockchain Dashboard</h1>"
            "<p>Dashboard läuft 🎉</p>"
            "<p>Endpoints:</p>"
            "<ul>"
            "<li><a style='color:#9cf' href='/chain/status'>/chain/status</a></li>"
            "<li><a style='color:#9cf' href='/dnd/state'>/dnd/state</a></li>"
            "<li><a style='color:#9cf' href='/metrics'>/metrics</a></li>"
            "</ul>"
            "</body></html>";

        res.set_content(html, "text/html");
    });

    // Simple debug page
    http.Get("/dashboard", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            "<html><body><h1>Blockchain Dashboard</h1><p>läuft 🎉</p></body></html>",
            "text/html"
        );
    });
}


#include "web/dashboardServer.hpp"
#include "web/chainApi.hpp"
#include "web/dndApi.hpp"

#include <thirdparty/httplib.h>
#include <filesystem>
#include <iostream>

DashboardServer::DashboardServer(int port,
                                 const std::string& reportsDir,
                                 const std::string& binaryPath)
    : port(port), reportsDir(reportsDir), binaryPath(binaryPath)
{
}

void DashboardServer::start()
{
    namespace fs = std::filesystem;

    // Ordner erstellen falls er fehlt
    if (!fs::exists(reportsDir)) {
        std::cout << "[DashboardServer] Creating folder: " << reportsDir << "\n";
        fs::create_directories(reportsDir);
    }

    httplib::Server server;

    // ========== BASIC TEST ENDPOINTS ==========
    server.Get("/ping", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"pong\":true}", "application/json");
    });

    server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // ========== GOSSIP: TX ==========
    server.Post("/gossip/tx", [&](const httplib::Request& req,
                                 httplib::Response& res)
    {
        std::cout << "[GOSSIP] Received TX payload: " << req.body << "\n";

        res.status = 200;
        res.set_content("{\"received\":true}", "application/json");
    });

    // ========== GOSSIP: BLOCK ==========
    server.Post("/gossip/block", [&](const httplib::Request& req,
                                    httplib::Response& res)
    {
        std::cout << "[GOSSIP] Received BLOCK payload: " << req.body << "\n";

        res.status = 200;
        res.set_content("{\"received\":true}", "application/json");
    });

    // ========== ROOT PAGE ==========
    server.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        std::string html = "<html><body><h1>Blockchain Dashboard</h1>"
                           "<p>Server läuft.</p></body></html>";
        res.set_content(html, "text/html");
    });

    // ========== START SERVER ==========
    std::cout << "[DashboardServer] Listening on 0.0.0.0:" << port << "\n";
    server.listen("0.0.0.0", port);
}


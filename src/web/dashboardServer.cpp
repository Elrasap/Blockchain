#include "web/dashboardServer.hpp"
#include <thirdparty/httplib.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "web/dndApi.hpp"
#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "core/blockBuilder.hpp"
#include "web/dndApi.hpp"
#include "web/dmApi.hpp"
#include "web/chainApi.hpp"

web::DndApi dndApi(
    server,
    ctx.chain,          // Blockchain
    ctx.mempool,        // Mempool
    ctx.dmPrivKey,      // DM private key
    ctx.dmPubKey        // DM public key
);

// …
DndApi dndApi(mempool, dndValidator);
dndApi.attach(server);


DashboardServer::DashboardServer(int port,
                                 const std::string& reportsDir,
                                 const std::string& binaryPath)
    : port(port), reportsDir(reportsDir), binaryPath(binaryPath) {}

void DashboardServer::start() {
    namespace fs = std::filesystem;

    // === FIX: Ordner automatisch erstellen ===
    if (!fs::exists(reportsDir)) {
        std::cout << "[DashboardServer] Creating missing directory: "
                  << reportsDir << "\n";
        fs::create_directories(reportsDir);
    }

    httplib::Server server;

    server.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        std::string html = "<h1>Blockchain Dashboard</h1><ul>";

        for (const auto& entry : fs::directory_iterator(reportsDir)) {
            if (entry.is_regular_file()) {
                auto name = entry.path().filename().string();
                html += "<li><a href=\"/report/" + name + "\">" + name + "</a></li>";
            }
        }

        html += "</ul>";
        res.set_content(html, "text/html");
    });
    server.Post("/gossip/tx", [&](const Request& req, Response& res) {
        auto j = json::parse(req.body);
        Transaction tx;
        tx.senderPubkey = j["sender"];
        tx.payload      = j["payload"];
        tx.signature    = j["signature"];

        std::string err;
        mempool.addTransactionValidated(tx, err);

        res.set_content("OK", "text/plain");
    });

    server.Post("/gossip/block", [&](const Request& req, Response& res) {
        auto j = json::parse(req.body);
        Block b = Block::deserialize(req.body);

        chain.appendBlock(b);

        res.set_content("OK", "text/plain");
    });

    server.Get(R"(/report/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string filePath = reportsDir + "/" + req.matches[1].str();

        std::ifstream f(filePath);
        if (!f.is_open()) {
            res.status = 404;
            res.set_content("Not found", "text/plain");
            return;
        }

        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());

        res.set_content(content, "text/html");
    });

    std::cout << "[DashboardServer] Listening on 0.0.0.0:" << port << "\n";

    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "[DashboardServer] ERROR: Could not bind port " << port << "\n";
    }
}


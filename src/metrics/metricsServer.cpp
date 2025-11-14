#include "obs/metricsServer.hpp"
#include "obs/metrics.hpp"
#include "obs/healthChecker.hpp"
#include <thread>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

MetricsServer::MetricsServer(int p) : port(p), running(false) {}

#include <httplib.h>
#include <thread>
#include <atomic>
#include <iostream>

MetricsServer::MetricsServer(int port)
    : port(port), running(false) {}

void MetricsServer::start() {
    if(running) return;
    running = true;

    serverThread = std::thread([this]() {
        try {
            httplib::Server svr;

            svr.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
                std::string body;
                body += "blockchain_health 1\n";
                body += "rto_average_ms 5000\n";
                body += "transactions_total 12345\n";
                res.set_content(body, "text/plain");
            });

            std::cout << "[MetricsServer] Running on :"
                      << port << "\n";

            svr.listen("0.0.0.0", port);

        } catch(const std::exception& e) {
            std::cerr << "[MetricsServer] Fatal: " << e.what() << "\n";
        }
        running = false;
    });
}

void MetricsServer::stop(){
    if(!running) return;
    running = false;
    if(serverThread.joinable())
        serverThread.join();
}

MetricsServer::~MetricsServer(){
    stop();
}


void MetricsServer::run() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return;
    }

    while (running) {
        sockaddr_in client;
        socklen_t len = sizeof(client);
        int client_fd = accept(server_fd, (sockaddr*)&client, &len);
        if (client_fd < 0) {
            if (!running) break;
            continue;
        }

        char buffer[2048];
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t read_bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        if (read_bytes <= 0) {
            close(client_fd);
            continue;
        }

        std::string req(buffer);
        std::string response_body;
        std::string content_type = "text/plain";

        if (req.find("GET /metrics") == 0) {
            response_body = Metrics::instance().renderPrometheus();
        } else if (req.find("GET /health") == 0) {
            response_body = HealthChecker::instance().statusJson();
            content_type = "application/json";
        } else {
            response_body = "Not Found";
        }

        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + content_type + "\r\n"
            "Content-Length: " + std::to_string(response_body.size()) + "\r\n"
            "Connection: close\r\n\r\n" +
            response_body;

        send(client_fd, response.c_str(), response.size(), 0);
        close(client_fd);
    }

    close(server_fd);
}


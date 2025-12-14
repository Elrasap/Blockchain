#pragma once

#include <string>

// Vorwärtsdeklaration, damit der Header nicht das ganze httplib reinzieht
namespace httplib {
    class Server;
}

class DashboardServer {
public:
    DashboardServer(int port,
                    const std::string& reportsDir,
                    const std::string& binaryPath);

    // Startet einen eigenen HTTP-Server nur für das Dashboard (wenn du ihn nutzen willst)
    void start();

    // Hängt das Dashboard an einen bestehenden httplib::Server an (so wie in main.cpp)
    void attach(httplib::Server& http);

private:
    int port;
    std::string reportsDir;
    std::string binaryPath;
};


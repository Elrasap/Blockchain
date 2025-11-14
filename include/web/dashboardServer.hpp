#pragma once
#include <string>

class DashboardServer {
public:
    DashboardServer(int port,
                    const std::string& reportsDir,
                    const std::string& binaryPath);

    void start();

private:
    int         port;
    std::string reportsDir;
    std::string binaryPath;
};


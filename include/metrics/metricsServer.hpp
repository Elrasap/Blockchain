#pragma once
#include <map>
#include <string>

class MetricsServer {
public:
    explicit MetricsServer(int port);
    void serve(const std::map<std::string,double>& metrics);
private:
    int port;
};


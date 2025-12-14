#pragma once
#include <string>
#include <vector>

struct Config {
    int httpPort = 8080;
    int gossipPort = 8090;

    std::vector<uint8_t> dmPrivKey;
    std::vector<uint8_t> dmPubKey;

    std::vector<std::string> peers;

    static Config load(const std::string& path);
};


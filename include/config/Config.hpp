#pragma once
#include <string>
#include <vector>

struct Config {
    int httpPort = 8080;
    int gossipPort = 8090;

    std::string dmKeyFile = "keys/dm.key";

    std::vector<std::string> peers;

    static Config load(const std::string& path);
};

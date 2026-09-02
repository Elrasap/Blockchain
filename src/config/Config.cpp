#include "config/Config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

Config Config::load(const std::string& path)
{
    Config cfg;

    std::ifstream f(path);
    if (!f) {
        std::cerr << "[Config] Could not read " << path << "\n";
        return cfg;
    }

    json j;
    f >> j;

    cfg.httpPort   = j.value("httpPort", 8080);
    cfg.gossipPort = j.value("gossipPort", 8090);

    cfg.dmKeyFile  = j.value("dmKeyFile", "keys/dm.key");

    cfg.peers      = j.value("peers", std::vector<std::string>{});

    return cfg;
}

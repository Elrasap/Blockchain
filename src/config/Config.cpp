#include "config/Config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

static std::vector<uint8_t> hexToBytes(const std::string& hex)
{
    std::vector<uint8_t> out;

    for (size_t i = 0; i < hex.size(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        uint8_t b = static_cast<uint8_t>(strtol(byteStr.c_str(), nullptr, 16));
        out.push_back(b);
    }

    return out;
}

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

    cfg.dmPrivKey  = hexToBytes(j.value("dmPrivKey", ""));
    cfg.dmPubKey   = hexToBytes(j.value("dmPubKey", ""));

    cfg.peers      = j.value("peers", std::vector<std::string>{});

    return cfg;
}


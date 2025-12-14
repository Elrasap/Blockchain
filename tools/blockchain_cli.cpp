#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "config/Config.hpp"
#include "thirdparty/httplib.h"

using json = nlohmann::json;

void cmd_status(const Config& cfg)
{
    httplib::Client cli("localhost", cfg.httpPort);
    auto res = cli.Get("/chain/height");
    if (!res) {
        std::cout << "Node offline\n";
        return;
    }
    std::cout << res->body << "\n";
}

void cmd_peers(const Config& cfg)
{
    httplib::Client cli("localhost", cfg.httpPort);
    auto res = cli.Get("/peers");
    if (!res) {
        std::cout << "Node offline\n";
        return;
    }
    std::cout << res->body << "\n";
}

void cmd_send_tx(const Config& cfg, const std::string& jsonFile)
{
    std::ifstream f(jsonFile);
    if (!f) {
        std::cerr << "Cannot open " << jsonFile << "\n";
        return;
    }

    json j;
    f >> j;

    httplib::Client cli("localhost", cfg.httpPort);
    auto res = cli.Post("/gossip/tx", j.dump(), "application/json");

    if (!res) {
        std::cout << "Error sending TX\n";
        return;
    }

    std::cout << res->body << "\n";
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Commands:\n"
                  << "  status\n"
                  << "  peers\n"
                  << "  send-tx <file.json>\n";
        return 0;
    }

    Config cfg = Config::load("config.json");

    std::string cmd = argv[1];

    if (cmd == "status") {
        cmd_status(cfg);
    }
    else if (cmd == "peers") {
        cmd_peers(cfg);
    }
    else if (cmd == "send-tx" && argc >= 3) {
        cmd_send_tx(cfg, argv[2]);
    }
    else {
        std::cout << "Unknown command\n";
    }

    return 0;
}


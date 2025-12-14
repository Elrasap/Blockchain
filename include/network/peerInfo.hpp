#pragma once
#include <string>
#include <cstdint>

struct PeerInfo {
    std::string address;   // "127.0.0.1:9001"
    uint64_t lastSeen = 0; // unix timestamp
    uint64_t height   = 0; // remote node height
};


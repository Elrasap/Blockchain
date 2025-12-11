#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct DmKeyPair {
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> privateKey;
};

bool loadOrCreateDmKey(const std::string& path, DmKeyPair& out);


#pragma once
#include <map>
#include <string>
#include <vector>
#include <array>
#include "crypto/keypair.hpp"
#include "crypto/types.hpp"

class KVStore {
public:
    bool put(const std::array<uint8_t, 32>& key,
             const std::vector<uint8_t>& value);
    std::vector<uint8_t> get(const std::array<uint8_t, 32>& key) const;
    void del(const std::array<uint8_t, 32>& key);
    bool batchWrite(const std::vector<std::pair<std::array<uint8_t, 32>, std::vector<uint8_t>>>& entries);
    void compact();
};


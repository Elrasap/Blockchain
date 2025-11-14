#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include "core/block.hpp"
#include "core/crypto.hpp"

class StateValidator {
public:
    static std::array<uint8_t,32> computeStateRoot(const std::vector<Block>& blocks);
    static bool compareSnapshots(const std::string& snapAPath, const std::string& snapBPath);
    static bool verifyStateEquality(const std::vector<Block>& chainA, const std::vector<Block>& chainB);
};


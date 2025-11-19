#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>

class Block;

class StateValidator {
public:
    // Deterministischer Chain-State-Root
    static std::array<uint8_t,32>
    computeStateRoot(const std::vector<Block>& blocks);

    // Datei A == Datei B?
    static bool compareSnapshots(const std::string& snapAPath,
                                 const std::string& snapBPath);

    // Chains logisch identisch?
    static bool verifyStateEquality(const std::vector<Block>& chainA,
                                    const std::vector<Block>& chainB);
};


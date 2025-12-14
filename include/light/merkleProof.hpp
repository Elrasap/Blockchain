#pragma once
#include <vector>
#include <array>
#include <cstdint>

struct MerkleProof {
    std::array<uint8_t,32> leaf;
    std::vector<std::array<uint8_t,32>> path;
    std::vector<bool> left;
    std::array<uint8_t,32> root;
};

std::array<uint8_t,32> computeMerkleRootFromProof(const MerkleProof& p);
bool verifyMerkleProof(const MerkleProof& p);


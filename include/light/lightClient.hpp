#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include "light/headerChain.hpp"
#include "light/merkleProof.hpp"

class LightClient {
public:
    bool ingestHeader(const BlockHeader& h);
    bool verifyTxInBlock(const MerkleProof& proof, const std::array<uint8_t,32>& expectedHeaderHash);
    std::array<uint8_t,32> head() const;
    uint64_t height() const;
    void reset();
private:
    HeaderChain headers;
};


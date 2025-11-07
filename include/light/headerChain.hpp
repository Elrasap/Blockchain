#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include "core/block.hpp"
#include "core/crypto.hpp"

class HeaderChain {
public:
    bool addHeader(const BlockHeader& h);
    bool validateLink(const BlockHeader& prev, const BlockHeader& curr) const;
    std::array<uint8_t,32> headHash() const;
    uint64_t height() const;
    const std::vector<BlockHeader>& headers() const;
    void clear();
private:
    std::vector<BlockHeader> chain;
};


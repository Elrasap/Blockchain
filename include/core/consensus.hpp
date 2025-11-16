#pragma once
#include <vector>
#include <cstdint>
#include "core/block.hpp"

// PoA-Manager verwaltet DM-Key
class PoAValidator {
public:
    PoAValidator(const std::vector<uint8_t>& dmPubKey);

    bool validateBlockHeader(const BlockHeader& header) const;

private:
    std::vector<uint8_t> dmPubKey;
};


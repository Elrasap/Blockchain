#pragma once
#include "crypto/hash.hpp"

class Sha256Hash : public IHashAlgorithm {
public:
    std::string name() const override;
    size_t digest_size() const override;
    std::vector<uint8_t> hash(const std::vector<uint8_t>& data) const override;
};


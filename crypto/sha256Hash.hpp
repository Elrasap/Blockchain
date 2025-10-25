#pragma once
#include "hash.hpp"
#include "types.hpp"
#include <vector>
#include <string>

class SHA256Hash : public IHashAlgorithm {
public:
    std::string name() const override;
    size_t digestSize() const override;
    std::array<uint8_t, 32> hash(const std::vector<uint8_t>& data) const override;
};

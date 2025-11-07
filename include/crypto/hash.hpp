#pragma once
#include <vector>
#include <string>
#include <cstdint>   // wichtig! wegen uint8_t

class IHashAlgorithm {
public:
    virtual ~IHashAlgorithm() = default;
    virtual std::string name() const = 0;
    virtual size_t digest_size() const = 0;
    virtual std::vector<uint8_t> hash(const std::vector<uint8_t>& data) const = 0;
};


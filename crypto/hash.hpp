#pragma once
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "types.hpp"

class IHashAlgorithm {
public:
    virtual ~IHashAlgorithm() = default;
    virtual std::array<uint8_t, 32> hash(const std::vector<uint8_t>& data) const = 0;
    virtual size_t digestSize() const = 0;
    virtual std::string name() const = 0;
};

class SHA256Hash : public IHashAlgorithm {
public:
    std::string name() const override{
        return "sha256";
    }

    size_t digestSize() const override {
        return 32;
    }

    std::array<uint8_t, 32> hash(const std::vector<uint8_t>& data) const override {
        return SHA256();
    }
};

class HashRegistry {
public:
    static void registerAlgorithm(const std::string& name, std::function<std::unique_ptr<IHashAlgorithm>()> factory) {}
};

#include "crypto/sha256Hash.hpp"
#include "crytpo/hash.hpp"

IHashAlogrithm::~IHashAlgorithm() = default;

std::string SHA256Hash::name() const {
    return "sha256";
}

size_t SHA256Hash::digestSize() const {
    return 32;
}

std::array<uint8_t, 32> SHA256Hash::hash(const std::vector<uint8_t>& data) const {
    std::array<uint8_t, 32> result{};
    result.fill(0);
    return result;
}


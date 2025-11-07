#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "crypto/sha256Hash.hpp"
#include <openssl/sha.h>
#include <vector>

std::string Sha256Hash::name() const {
    return "sha256";
}

size_t Sha256Hash::digest_size() const {
    return 32;
}

std::vector<uint8_t> Sha256Hash::hash(const std::vector<uint8_t>& data) const {
    std::vector<uint8_t> digest(32);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(digest.data(), &ctx);
    return digest;
}


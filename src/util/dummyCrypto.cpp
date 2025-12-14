#include "util/dummyCrypto.hpp"

std::string fileSha256Hex(const std::string&) {
    return "00";  // Dummy
}

std::vector<uint8_t> fromHex(const std::string&) {
    return {};    // Dummy
}

bool verifySignatureOverFile(const std::string&,
                             const std::vector<uint8_t>&,
                             const std::vector<uint8_t>&) {
    return true;  // Dummy always succeeds
}


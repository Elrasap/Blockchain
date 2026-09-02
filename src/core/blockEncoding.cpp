#include "core/blockEncoding.hpp"
#include <stdexcept>

std::vector<uint8_t> encodeHeader(const BlockHeader& h) {
    return h.toBytes();
}

BlockHeader decodeHeader(const std::vector<uint8_t>& buf) {
    BlockHeader h{};
    std::string error;
    if (!BlockHeader::deserialize(buf, h, error))
        throw std::runtime_error(error);
    return h;
}

#include "core/blockEncoding.hpp"
#include <cstring>

std::vector<uint8_t> encodeHeader(const BlockHeader& h) {
    std::vector<uint8_t> buf;

    buf.insert(buf.end(), h.prevHash.begin(), h.prevHash.end());
    buf.insert(buf.end(), h.merkleRoot.begin(), h.merkleRoot.end());

    for (int i = 0; i < 8; i++) buf.push_back((h.height    >> (i*8)) & 0xFF);
    for (int i = 0; i < 8; i++) buf.push_back((h.timestamp >> (i*8)) & 0xFF);

    return buf;
}

BlockHeader decodeHeader(const std::vector<uint8_t>& buf) {
    BlockHeader h{};
    size_t off = 0;

    memcpy(h.prevHash.data(), buf.data() + off, 32); off += 32;
    memcpy(h.merkleRoot.data(), buf.data() + off, 32); off += 32;

    memcpy(&h.height,    buf.data() + off, 8); off += 8;
    memcpy(&h.timestamp, buf.data() + off, 8); off += 8;

    return h;
}


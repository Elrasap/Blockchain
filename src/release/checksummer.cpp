#include "release/checksummer.hpp"
#include <fstream>
#include <vector>
#include "core/crypto.hpp"

std::array<uint8_t,32> fileSha256(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> buf;
    std::vector<uint8_t> chunk(4096);
    while (in) {
        in.read(reinterpret_cast<char*>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
        std::streamsize got = in.gcount();
        if (got > 0) buf.insert(buf.end(), chunk.begin(), chunk.begin() + got);
    }
    return crypto::sha256(buf);
}

std::string fileSha256Hex(const std::string& path) {
    auto h = fileSha256(path);
    return crypto::toHex(h);
}


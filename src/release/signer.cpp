#include "release/signer.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

std::vector<uint8_t> signDigest(const std::array<uint8_t,32>& digest, const std::vector<uint8_t>& priv) {
    std::vector<uint8_t> msg(digest.begin(), digest.end());
    return crypto::sign(msg, priv);
}

bool writeSignatureFile(const std::string& sigPath, const std::vector<uint8_t>& sig) {
    std::ofstream out(sigPath, std::ios::binary);
    if (!out) return false;
    std::string hex = toHex(sig);
    out.write(hex.data(), static_cast<std::streamsize>(hex.size()));
    return true;
}

std::vector<uint8_t> readSignatureFile(const std::string& sigPath) {
    std::ifstream in(sigPath, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return fromHex(ss.str());
}

std::string toHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : data) oss << std::setw(2) << static_cast<int>(b);
    return oss.str();
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::vector<uint8_t> fromHex(const std::string& hex) {
    std::vector<uint8_t> out;
    size_t n = hex.size();
    for (size_t i = 0; i + 1 < n; i += 2) {
        int hi = hexval(hex[i]);
        int lo = hexval(hex[i+1]);
        if (hi < 0 || lo < 0) break;
        out.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return out;
}


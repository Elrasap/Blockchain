#include "light/headerChain.hpp"
#include "core/crypto.hpp"
#include <vector>

static std::vector<uint8_t> serializeHeader(const BlockHeader& h) {
    std::vector<uint8_t> buf;
    buf.insert(buf.end(), h.prevHash.begin(), h.prevHash.end());
    buf.insert(buf.end(), h.merkleRoot.begin(), h.merkleRoot.end());
    for (int i = 0; i < 8; ++i) buf.push_back((h.height >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.timestamp >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.nonce >> (i * 8)) & 0xFF);
    return buf;
}

bool HeaderChain::validateLink(const BlockHeader& prev, const BlockHeader& curr) const {
    std::vector<uint8_t> prevBytes = serializeHeader(prev);
    auto prevHash = sha256(prevBytes);
    return curr.prevHash == prevHash;
}

bool HeaderChain::addHeader(const BlockHeader& h) {
    if (chain.empty()) {
        chain.push_back(h);
        return true;
    }
    const BlockHeader& prev = chain.back();
    if (validateLink(prev, h)) {
        chain.push_back(h);
        return true;
    }
    return false;
}

std::array<uint8_t, 32> HeaderChain::headHash() const {
    if (chain.empty()) return {};
    std::vector<uint8_t> bytes = serializeHeader(chain.back());
    return sha256(bytes);
}

uint64_t HeaderChain::height() const {
    if (chain.empty()) return 0;
    return chain.back().height;
}

const std::vector<BlockHeader>& HeaderChain::headers() const {
    return chain;
}

void HeaderChain::clear() {
    chain.clear();
}


#include "light/headerChain.hpp"
#include "core/crypto.hpp"
#include <vector>

//
// Serialize BlockHeader
//
static std::vector<uint8_t> serializeHeader(const BlockHeader& h) {
    std::vector<uint8_t> buf;
    buf.reserve(32 + 32 + 8 + 8 + 8);

    buf.insert(buf.end(), h.prevHash.begin(), h.prevHash.end());
    buf.insert(buf.end(), h.merkleRoot.begin(), h.merkleRoot.end());

    for (int i = 0; i < 8; ++i) buf.push_back((h.height    >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.timestamp >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) buf.push_back((h.nonce     >> (i * 8)) & 0xFF);

    return buf;
}

//
// Validate: curr.prevHash == sha256(prevHeader)
//
bool HeaderChain::validateLink(const BlockHeader& prev, const BlockHeader& curr) const {
    std::vector<uint8_t> prevBytes = serializeHeader(prev);
    auto prevHash = crypto::sha256(prevBytes);
    return curr.prevHash == prevHash;
}

//
// Add header if valid link
//
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

//
// Hash of the head block
//
std::array<uint8_t, 32> HeaderChain::headHash() const {
    if (chain.empty()) return {};

    std::vector<uint8_t> bytes = serializeHeader(chain.back());
    return crypto::sha256(bytes);
}

//
// Current height
//
uint64_t HeaderChain::height() const {
    if (chain.empty()) return 0;
    return chain.back().height;
}

//
// All headers (for sync or debug)
//
const std::vector<BlockHeader>& HeaderChain::headers() const {
    return chain;
}

//
// Reset
//
void HeaderChain::clear() {
    chain.clear();
}


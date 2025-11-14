#include "light/lightClient.hpp"
#include "core/crypto.hpp"
#include <vector>

//
// Helper: serialize block header (same as in headerChain.cpp)
//
static std::vector<uint8_t> encodeHeader(const BlockHeader& h) {
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
// Ingest header into chain
//
bool LightClient::ingestHeader(const BlockHeader& h) {
    return headers.addHeader(h);
}

//
// Verify a merkle proof AND confirm that the block header matches expected hash
//
bool LightClient::verifyTxInBlock(const MerkleProof& proof,
                                  const std::array<uint8_t,32>& expectedHeaderHash)
{
    // First check Merkle proof
    if (!verifyMerkleProof(proof))
        return false;

    // Now confirm block header hash
    for (const auto& hdr : headers.headers()) {
        std::vector<uint8_t> encoded = encodeHeader(hdr);
        auto hash = crypto::sha256(encoded);

        if (hash == expectedHeaderHash)
            return true;
    }

    return false;
}

//
// Current head hash
//
std::array<uint8_t,32> LightClient::head() const {
    return headers.headHash();
}

//
// Current height
//
uint64_t LightClient::height() const {
    return headers.height();
}

//
// Reset all headers
//
void LightClient::reset() {
    headers.clear();
}


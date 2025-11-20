#include "light/lightClient.hpp"
#include "core/crypto.hpp"
#include <vector>
#include "core/blockEncoding.hpp"

//
// Helper: serialize block header (same as in headerChain.cpp)
//
#include "light/lightClient.hpp"
#include "core/block.hpp"
#include <vector>

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


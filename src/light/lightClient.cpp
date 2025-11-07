#include "light/lightClient.hpp"

bool LightClient::ingestHeader(const BlockHeader& h) {
    return headers.addHeader(h);
}

bool LightClient::verifyTxInBlock(const MerkleProof& proof, const std::array<uint8_t,32>& expectedHeaderHash) {
    if (!verifyMerkleProof(proof)) return false;
    for (const auto& hdr : headers.headers()) {
        if (sha256(encodeHeader(hdr)) == expectedHeaderHash) return true;
    }
    return false;
}

std::array<uint8_t,32> LightClient::head() const {
    return headers.headHash();
}

uint64_t LightClient::height() const {
    return headers.height();
}

void LightClient::reset() {
    headers.clear();
}


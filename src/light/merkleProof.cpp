#include "light/merkleProof.hpp"
#include "core/crypto.hpp"
#include <vector>

std::array<uint8_t, 32> computeMerkleRootFromProof(const MerkleProof& p) {
    std::array<uint8_t, 32> h = p.leaf;
    for (size_t i = 0; i < p.path.size(); ++i) {
        std::vector<uint8_t> buf;
        buf.reserve(64);
        if (p.left[i]) {
            buf.insert(buf.end(), p.path[i].begin(), p.path[i].end());
            buf.insert(buf.end(), h.begin(), h.end());
        } else {
            buf.insert(buf.end(), h.begin(), h.end());
            buf.insert(buf.end(), p.path[i].begin(), p.path[i].end());
        }
        h = sha256(buf);
    }
    return h;
}

bool verifyMerkleProof(const MerkleProof& p) {
    auto root = computeMerkleRootFromProof(p);
    return root == p.root;
}


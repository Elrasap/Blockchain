#include "core/validation.hpp"
#include "core/block.hpp"

using namespace std;

namespace Validation {

bool validateTransaction(const Transaction& tx) {
    // Aktuell: nur Signatur prüfen.
    // Später kannst du hier Nonce, Fee, Policy etc. prüfen.
    return tx.verifySignature();
}

bool validateBlock(const Block& block, const Block& prev) {
    // Höhe muss inkrementieren
    if (block.header.height != prev.header.height + 1) {
        return false;
    }

    // prevHash muss auf den Vorgänger zeigen
    if (block.header.prevHash != prev.hash()) {
        return false;
    }

    // Merkle-Root prüfen
    if (block.header.merkleRoot != block.calculateMerkleRoot()) {
        return false;
    }

    // Header-Signatur (falls gesetzt)
    if (!block.header.validatorPubKey.empty() ||
        !block.header.signature.empty()) {
        if (!verifyBlockHeaderSignature(block.header)) {
            return false;
        }
    }

    return true;
}

bool validateBlockPoA(const Block& block,
                      const Block& prev,
                      const std::vector<uint8_t>& expectedValidatorPubKey) {

    if (!validateBlock(block, prev)) {
        return false;
    }

    // Proof-of-Authority: nur der erwartete Validator-Key ist erlaubt.
    if (block.header.validatorPubKey != expectedValidatorPubKey) {
        return false;
    }

    return true;
}

} // namespace Validation


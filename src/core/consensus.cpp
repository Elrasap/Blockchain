#include "core/consensus.hpp"
#include "core/block.hpp"
#include "core/crypto.hpp"
#include <iostream>

PoAValidator::PoAValidator(const std::vector<uint8_t>& dmPubKey)
    : dmPubKey(dmPubKey)
{}

bool PoAValidator::validateBlockHeader(const BlockHeader& header) const {

    // === Check 1: validator == DM Pubkey ===
    if (header.validatorPubKey != dmPubKey) {
        std::cerr << "[PoA] Block validatorPubKey does not match DM key!\n";
        return false;
    }

    // === Check 2: Signature present? ===
    if (header.signature.empty()) {
        std::cerr << "[PoA] Missing block signature!\n";
        return false;
    }

    // === Check 3: Signature valid? ===
    auto msg = header.toBytes();

    if (!crypto::verify(msg, header.signature, header.validatorPubKey)) {
        std::cerr << "[PoA] Invalid block signature!\n";
        return false;
    }

    return true;
}


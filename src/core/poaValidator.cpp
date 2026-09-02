#include "core/poaValidator.hpp"
#include <sodium.h>
#include <iostream>

// --------------------------------------------------------
// Sign
// --------------------------------------------------------
bool signBlockHeader(BlockHeader& header,
                     const std::vector<uint8_t>& privKey,
                     const std::vector<uint8_t>& pubKey)
{
    if (privKey.size() != crypto_sign_SECRETKEYBYTES ||
        pubKey.size()  != crypto_sign_PUBLICKEYBYTES)
    {
        std::cerr << "[PoA] Invalid key sizes\n";
        return false;
    }

    header.validatorPubKey = pubKey;

    auto msg = header.toSigningBytes();

    header.signature.resize(crypto_sign_BYTES);
    if (crypto_sign_detached(header.signature.data(),
                             nullptr,
                             msg.data(),
                             msg.size(),
                             privKey.data()) != 0)
    {
        std::cerr << "[PoA] Signature failed\n";
        return false;
    }

    return true;
}

// --------------------------------------------------------
// Verify
// --------------------------------------------------------
bool verifyBlockHeaderSignature(const BlockHeader& header)
{
    if (header.validatorPubKey.size() != crypto_sign_PUBLICKEYBYTES ||
        header.signature.size()      != crypto_sign_BYTES)
    {
        std::cerr << "[PoA] Invalid header key/sig sizes\n";
        return false;
    }

    auto msg = header.toSigningBytes();

    if (crypto_sign_verify_detached(header.signature.data(),
                                    msg.data(),
                                    msg.size(),
                                    header.validatorPubKey.data()) != 0)
    {
        std::cerr << "[PoA] Signature invalid\n";
        return false;
    }

    return true;
}

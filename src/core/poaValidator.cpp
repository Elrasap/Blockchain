#include "core/poaValidator.hpp"
#include <sodium.h>
#include <cstring>
#include <iostream>

// --------------------------------------------------------
// Serialize header (without signature)
// --------------------------------------------------------
static std::vector<uint8_t> serializeHeaderForSign(const BlockHeader& h)
{
    std::vector<uint8_t> data;
    data.reserve(8 + 32 + 32 + 8 + 32);

    // height
    for (int i = 0; i < 8; i++)
        data.push_back((h.height >> (i * 8)) & 0xFF);

    // prevHash
    data.insert(data.end(), h.prevHash.begin(), h.prevHash.end());

    // merkleRoot
    data.insert(data.end(), h.merkleRoot.begin(), h.merkleRoot.end());

    // timestamp
    for (int i = 0; i < 8; i++)
        data.push_back((h.timestamp >> (i * 8)) & 0xFF);

    // validatorPubKey
    data.insert(data.end(), h.validatorPubKey.begin(), h.validatorPubKey.end());

    return data;
}

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

    auto msg = serializeHeaderForSign(header);

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

    auto msg = serializeHeaderForSign(header);

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


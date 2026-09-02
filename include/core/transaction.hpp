#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <string>

class Transaction {
public:
    static constexpr std::size_t PUBLIC_KEY_SIZE = 32;
    static constexpr std::size_t SIGNATURE_SIZE = 64;
    static constexpr std::size_t MAX_PAYLOAD_SIZE = 64 * 1024;

    std::vector<uint8_t> senderPubkey;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> signature;

    uint64_t nonce = 0;
    uint64_t fee = 0;

    // Canonical wire representation, including the signature.
    std::vector<uint8_t> serialize() const;

    // Canonical body covered by the Ed25519 signature.
    std::vector<uint8_t> serializeForSigning() const;

    void sign(const std::vector<uint8_t>& priv);

    bool verifySignature() const;

    std::array<uint8_t, 32> hash() const;

    void deserialize(const std::vector<uint8_t>& data);
    static bool deserialize(const std::vector<uint8_t>& data,
                            Transaction& out,
                            std::string& error);
};

enum TxType {
    TX_TRANSFER = 0,
    TX_STAKE = 1,

    TX_DND_CREATE_CHARACTER = 50,
    TX_DND_UPDATE_CHARACTER = 51,

    TX_DND_ATTACK = 60,
    TX_DND_SKILL_CHECK = 61,
    TX_DND_SAVING_THROW = 62,
    TX_DND_INITIATIVE = 63
};

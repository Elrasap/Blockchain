#pragma once

#include <vector>
#include <array>
#include <cstdint>

class Transaction {
public:
    std::vector<uint8_t> senderPubkey;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> signature;

    uint64_t nonce = 0;
    uint64_t fee = 0;

    std::vector<uint8_t> serialize() const;

    // SIGNIERT MIT PRIVATKEY (1 Parameter!)
    void sign(const std::vector<uint8_t>& priv);

    bool verifySignature() const;

    std::array<uint8_t, 32> hash() const;

    void deserialize(const std::vector<uint8_t>& data);
};

enum TxType {
    TX_TRANSFER = 0,
    TX_STAKE = 1,

    TX_DND_CREATE_CHARACTER = 50,
    TX_DND_UPDATE_CHARACTER = 51,

    // --- Combat ---
    TX_DND_ATTACK = 60,
    TX_DND_SKILL_CHECK = 61,
    TX_DND_SAVING_THROW = 62,
    TX_DND_INITIATIVE = 63
};


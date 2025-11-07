#pragma once
#include <string>
#include <vector>
#include <array>
#include "core/transaction.hpp"
#include "core/block.hpp"
#include "light/merkleProof.hpp"

enum class MessageType {
    TX_BROADCAST = 1,
    PING = 2,
    PONG = 3,
    INV = 4,
    GETBLOCK = 5,
    BLOCK = 6,
    APPEND_ENTRIES = 7,
    ACK = 8,
    HEADER = 9,
    GETHEADER = 10,
    PROOF_TX = 11,
    GETPROOF_TX = 12
};

// ---- Message Struct muss vor Funktionsdeklarationen stehen ----
struct Message {
    MessageType type;
    std::vector<uint8_t> payload;
};

// ---- Funktionen für generische Message-Encoding ----
std::vector<uint8_t> encodeMessage(const Message& msg);
Message decodeMessage(const std::vector<uint8_t>& bytes);

// ---- Header-spezifisch ----
std::vector<uint8_t> encodeHeader(const BlockHeader& h);
BlockHeader decodeHeader(const std::vector<uint8_t>& bytes);

// ---- Proof-spezifisch ----
std::vector<uint8_t> encodeMerkleProof(const MerkleProof& p);
MerkleProof decodeMerkleProof(const std::vector<uint8_t>& bytes);

// ---- GETHEADER / GETPROOF_TX ----
std::vector<uint8_t> encodeGetHeader(uint64_t height);
uint64_t decodeGetHeader(const std::vector<uint8_t>& bytes);

std::vector<uint8_t> encodeGetProofTx(const std::array<uint8_t,32>& txHash);
std::array<uint8_t,32> decodeGetProofTx(const std::vector<uint8_t>& bytes);


#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <array>

namespace dnd {

struct DndEventTx {
    std::string encounterId;

    std::string actorId;
    std::string targetId;

    int actorType = 0;   // 0 = Character, 1 = Monster
    int targetType = 0;  // 0 = Character, 1 = Monster

    int roll = 0;
    int damage = 0;
    bool hit = false;
    std::string note;

    uint64_t timestamp = 0;

    std::vector<uint8_t> senderPubKey;
    std::vector<uint8_t> signature;
};

// ---- SIGNING API ----
bool generatePlayerKeypair(std::vector<uint8_t>& pubOut,
                           std::vector<uint8_t>& privOut);

void signDndEvent(DndEventTx& evt,
                  const std::vector<uint8_t>& privKey);

bool verifyDndEventSignature(const DndEventTx& evt,
                             std::string& err);

} // namespace dnd


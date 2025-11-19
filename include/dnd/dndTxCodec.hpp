#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace dnd {

struct DndEventTx;

// Ist der Payload JSON und enthält ein "encounterId"? → DnD TX
bool isDndPayload(const std::vector<uint8_t>& payload);

// DnDEventTx ↔ JSON-binary
std::vector<uint8_t> encodeDndTx(const DndEventTx& tx);
DndEventTx decodeDndTx(const std::vector<uint8_t>& buf);

} // namespace dnd


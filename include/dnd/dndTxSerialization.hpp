#pragma once
#include <vector>
#include <string>
#include "dnd/dndTx.hpp"

namespace dnd {

// Serialisiert ein DndEventTx in ein Byte-Array
std::vector<uint8_t> serializeDndTx(const DndEventTx& evt);

// Deserialisiert ein Byte-Array in ein DndEventTx
DndEventTx deserializeDndTx(const std::vector<uint8_t>& buf);

// Reines JSON-Debugging (für Tests)
std::string dndTxToJson(const DndEventTx& evt);

} // namespace dnd


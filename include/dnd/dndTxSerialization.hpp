#pragma once
#include <vector>
#include <string>
#include "dnd/dndTx.hpp"

namespace dnd {

std::vector<uint8_t> serializeDndTx(const DndEventTx& evt);

DndEventTx deserializeDndTx(const std::vector<uint8_t>& buf);

std::string dndTxToJson(const DndEventTx& evt);

}


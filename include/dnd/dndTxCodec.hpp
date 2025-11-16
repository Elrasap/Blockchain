#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "dnd/dndTx.hpp"

namespace dnd {

std::vector<uint8_t> encodeDndTx(const DndEventTx& tx);

DndEventTx decodeDndTx(const std::vector<uint8_t>& payload);

}


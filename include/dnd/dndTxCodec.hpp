#pragma once

#include <vector>
#include <cstdint>

#include "dnd/dndTx.hpp"

namespace dnd {

std::vector<uint8_t> encodeDndTx(const DndEventTx& tx);



DndEventTx decodeDndTx(const std::vector<uint8_t>& buf);

}


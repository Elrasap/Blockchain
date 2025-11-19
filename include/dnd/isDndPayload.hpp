#pragma once
#include <vector>
#include <cstdint>

namespace dnd {

inline bool isDndPayload(const std::vector<uint8_t>& buf)
{
    if (buf.empty()) return false;

    // Dein Codec beginnt IMMER mit einem Opcode für DnD.
    // Das value ist egal – wir nehmen nur "nicht 0".
    uint8_t opcode = buf[0];

    // 0 = kein DnD
    return opcode != 0;
}

} // namespace dnd


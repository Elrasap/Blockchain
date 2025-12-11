#pragma once
#include <vector>
#include <cstdint>

namespace dnd {

inline bool isDndPayload(const std::vector<uint8_t>& buf)
{
    if (buf.empty()) return false;

    uint8_t opcode = buf[0];


    return opcode != 0;
}

}


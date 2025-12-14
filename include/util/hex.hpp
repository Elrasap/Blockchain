#pragma once
#include <string>
#include <array>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace util {

inline std::string toHex(const std::array<uint8_t, 32>& hash)
{
    std::ostringstream oss;
    oss << "0x";

    for (uint8_t b : hash) {
        oss << std::hex
            << std::setw(2)
            << std::setfill('0')
            << (int)b;
    }

    return oss.str();
}

} // namespace util


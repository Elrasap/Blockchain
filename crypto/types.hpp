#pragma once
#include <array>
#include <cstdint>

namespace blockchain::crypto {
    using SHA256Digest = std::array<std::uint8_t, 32>;
    using Blake3 = std::array<std::uint8_t, 32>;
    using B32 = std::array<std::uint8_t, 32>;
    using B64 = std::array<std::uint8_t, 64>;
    using HashID = const char*;
}

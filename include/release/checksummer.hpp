#pragma once
#include <array>
#include <string>
#include <cstdint>

std::array<uint8_t,32> fileSha256(const std::string& path);
std::string fileSha256Hex(const std::string& path);


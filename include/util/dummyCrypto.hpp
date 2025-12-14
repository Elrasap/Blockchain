#pragma once
#include <string>
#include <vector>
#include <cstdint>

std::string fileSha256Hex(const std::string& path);
std::vector<uint8_t> fromHex(const std::string& hex);
bool verifySignatureOverFile(const std::string& file,
                             const std::vector<uint8_t>& sig,
                             const std::vector<uint8_t>& pub);


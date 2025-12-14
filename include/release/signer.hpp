#pragma once
#include <vector>
#include <string>
#include <array>
#include "core/crypto.hpp"

std::vector<uint8_t> signDigest(const std::array<uint8_t,32>& digest, const std::vector<uint8_t>& priv);
bool writeSignatureFile(const std::string& sigPath, const std::vector<uint8_t>& sig);
std::vector<uint8_t> readSignatureFile(const std::string& sigPath);
std::string toHex(const std::vector<uint8_t>& data);
std::vector<uint8_t> fromHex(const std::string& hex);


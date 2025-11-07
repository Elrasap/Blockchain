#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include "crypto/types.hpp"
#include "network/syncManager.hpp"

class SyncManager;
extern SyncManager* global_sync;

std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& data);
std::string toHex(const std::array<uint8_t, 32>& hash);

KeyPair generateKeyPair();
std::vector<uint8_t> sign(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& priv);
bool verify(const std::vector<uint8_t>& msg, const std::vector<uint8_t>& sig, const std::vector<uint8_t>& pub);

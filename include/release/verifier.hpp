#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>  // <── das fehlte

bool verifyChecksumHex(const std::string& path, const std::string& expectedHex);
bool verifySignatureOverFile(const std::string& path,
                             const std::vector<uint8_t>& signature,
                             const std::vector<uint8_t>& pubkey);


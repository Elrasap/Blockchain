#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct DmKeyPair {
    std::vector<uint8_t> publicKey;   // 32 Bytes
    std::vector<uint8_t> privateKey;  // 64 Bytes
};

// Lädt den DM-Key aus einer Datei oder erzeugt einen neuen, falls keiner existiert.
// Format der Datei (binär):
// [uint64 pubLen][uint64 privLen][pubBytes][privBytes]
bool loadOrCreateDmKey(const std::string& path, DmKeyPair& out);


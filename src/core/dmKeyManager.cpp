#include "core/dmKeyManager.hpp"
#include "core/crypto.hpp"

#include <fstream>
#include <iostream>

using namespace std;

namespace {

bool readUint64(ifstream& in, uint64_t& out) {
    in.read(reinterpret_cast<char*>(&out), sizeof(out));
    return static_cast<bool>(in);
}

bool writeUint64(ofstream& out, uint64_t v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(v));
    return static_cast<bool>(out);
}

} // namespace

bool loadOrCreateDmKey(const std::string& path, DmKeyPair& out) {
    // 1. Versuchen zu laden
    {
        ifstream in(path, ios::binary);
        if (in.is_open()) {
            uint64_t pubLen = 0;
            uint64_t privLen = 0;

            if (readUint64(in, pubLen) && readUint64(in, privLen) &&
                pubLen > 0 && privLen > 0 &&
                pubLen < (1ull << 20) && privLen < (1ull << 20)) {

                out.publicKey.resize(static_cast<size_t>(pubLen));
                out.privateKey.resize(static_cast<size_t>(privLen));

                in.read(reinterpret_cast<char*>(out.publicKey.data()), pubLen);
                in.read(reinterpret_cast<char*>(out.privateKey.data()), privLen);

                if (in) {
                    return true; // erfolgreich geladen
                }
            }

            std::cerr << "[DmKeyManager] Invalid key file, regenerating: " << path << "\n";
        }
    }

    // 2. Neu generieren
    try {
        auto kp = crypto::generateKeyPair();
        out.publicKey  = kp.publicKey;
        out.privateKey = kp.privateKey;
    } catch (const std::exception& ex) {
        std::cerr << "[DmKeyManager] Key generation failed: " << ex.what() << "\n";
        return false;
    }

    // 3. Speichern
    {
        ofstream outFile(path, ios::binary | ios::trunc);
        if (!outFile.is_open()) {
            std::cerr << "[DmKeyManager] Failed to open key file for writing: " << path << "\n";
            return false;
        }

        uint64_t pubLen  = out.publicKey.size();
        uint64_t privLen = out.privateKey.size();

        if (!writeUint64(outFile, pubLen) ||
            !writeUint64(outFile, privLen)) {
            std::cerr << "[DmKeyManager] Failed to write key lengths.\n";
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(out.publicKey.data()), pubLen);
        outFile.write(reinterpret_cast<const char*>(out.privateKey.data()), privLen);

        if (!outFile) {
            std::cerr << "[DmKeyManager] Failed to write key bytes.\n";
            return false;
        }
    }

    std::cout << "[DmKeyManager] Generated new DM key and stored at: " << path << "\n";
    return true;
}


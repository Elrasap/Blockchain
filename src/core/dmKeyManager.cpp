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
    static constexpr uint64_t DM_PUBKEY_BYTES  = 32;
    static constexpr uint64_t DM_PRIVKEY_BYTES = 64;

    // 1. Versuchen zu laden
    {
        ifstream in(path, ios::binary);
        if (in.is_open()) {
            uint64_t pubLen = 0;
            uint64_t privLen = 0;

            if (readUint64(in, pubLen) && readUint64(in, privLen) &&
                pubLen == DM_PUBKEY_BYTES &&
                privLen == DM_PRIVKEY_BYTES) {

                out.publicKey.resize(static_cast<size_t>(pubLen));
                out.privateKey.resize(static_cast<size_t>(privLen));

                in.read(reinterpret_cast<char*>(out.publicKey.data()), pubLen);
                in.read(reinterpret_cast<char*>(out.privateKey.data()), privLen);

                if (in) {
                    return true; // erfolgreich geladen
                }

                std::cerr << "[DmKeyManager] Failed to read key bytes from "
                          << path << " – regenerating.\n";
            } else {
                std::cerr << "[DmKeyManager] Invalid key lengths in " << path
                          << " (pub=" << pubLen << ", priv=" << privLen
                          << ") – regenerating.\n";
            }
        }
    }

    // 2. Neu generieren
    try {
        auto kp = crypto::generateKeyPair();
        out.publicKey  = kp.publicKey;
        out.privateKey = kp.privateKey;

        if (out.publicKey.size() != DM_PUBKEY_BYTES ||
            out.privateKey.size() != DM_PRIVKEY_BYTES) {
            std::cerr << "[DmKeyManager] Generated keypair has unexpected sizes "
                      << "(pub=" << out.publicKey.size()
                      << ", priv=" << out.privateKey.size() << ")\n";
            return false;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[DmKeyManager] Key generation failed: " << ex.what() << "\n";
        return false;
    }

    // 3. Speichern (unverändert wie bei dir)
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


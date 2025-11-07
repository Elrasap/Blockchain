#pragma once
#include <string>
#include <vector>
#include <memory>

class CryptoEngine {
public:
    void generateKeyPair();
    void rotateKeys();
    void exportPublicKey();
    void revokeOldKeys();
    void encryptForPeer(const Peer& p);
};

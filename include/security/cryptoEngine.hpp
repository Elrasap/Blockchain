#pragma once

class CryptoEngine {
public:
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce);
    std::vector<uint8_t> deriveSharedKey(const std::vector<uint8_t>& pub, const std::vector<uint8_t>& priv);
};

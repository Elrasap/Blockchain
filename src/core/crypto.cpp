#include "core/crypto.hpp"
#include <openssl/sha.h>
#include <sodium.h>
#include <string>
#include <array>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <sodium.h>
#include <stdexcept>

using namespace std;

array<uint8_t, 32> sha256(const vector<uint8_t>& input) {
    array<uint8_t, 32> out{};
    SHA256(input.data(), input.size(), out.data());
    return out;
}

string toHex(const array<uint8_t, 32>& hash) {
    static const char* hexDigits = "0123456789abcdef";
    string result;
    result.reserve(64);

    for (uint8_t byte : hash) {
        result.push_back(hexDigits[byte >> 4]);
        result.push_back(hexDigits[byte & 0x0F]);
    }

    return result;
}

KeyPair generateKeyPair() {
    if (sodium_init() < 0) throw runtime_error("libsodium init failed");
    KeyPair kp;
    kp.pub.resize(crypto_sign_PUBLICKEYBYTES);   // 32
    kp.priv.resize(crypto_sign_SECRETKEYBYTES);  // 64
    if (crypto_sign_keypair(kp.pub.data(), kp.priv.data()) != 0)
        throw runtime_error("keypair generation failed");
    return kp;
}

vector<uint8_t> sign(const vector<uint8_t>& msg,
                          const vector<uint8_t>& priv) {
    vector<uint8_t> sig(crypto_sign_BYTES);
    unsigned long long siglen = 0;
    if (crypto_sign_detached(sig.data(), &siglen, msg.data(), msg.size(), priv.data()) != 0)
        throw runtime_error("sign failed");
    sig.resize(siglen);
    return sig;
}

bool verify(const vector<uint8_t>& msg,
            const vector<uint8_t>& sig,
            const vector<uint8_t>& pub) {
    int rc = crypto_sign_verify_detached(sig.data(), msg.data(), msg.size(), pub.data());
    return rc == 0;
}


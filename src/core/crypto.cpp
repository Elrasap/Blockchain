#include "core/crypto.hpp"
#include <openssl/sha.h>
#include <sodium.h>
#include <stdexcept>
#include <string>

using namespace std;

namespace crypto {

array<uint8_t, 32> sha256(const vector<uint8_t>& input) {
    array<uint8_t, 32> out{};
    SHA256(input.data(), input.size(), out.data());
    return out;
}

string toHex(const array<uint8_t, 32>& hash) {
    static const char* HEX = "0123456789abcdef";
    string result;
    result.reserve(64);
    for (uint8_t b : hash) {
        result.push_back(HEX[b >> 4]);
        result.push_back(HEX[b & 0x0F]);
    }
    return result;
}

KeyPair generateKeyPair() {
    if (sodium_init() < 0)
        throw runtime_error("libsodium init failed");

    KeyPair kp;
    kp.publicKey.resize(crypto_sign_PUBLICKEYBYTES);
    kp.privateKey.resize(crypto_sign_SECRETKEYBYTES);

    if (crypto_sign_keypair(kp.publicKey.data(), kp.privateKey.data()) != 0)
        throw runtime_error("keypair generation failed");

    return kp;
}

vector<uint8_t> sign(const vector<uint8_t>& msg,
                     const vector<uint8_t>& priv) {

    vector<uint8_t> sig(crypto_sign_BYTES);
    unsigned long long siglen;

    if (crypto_sign_detached(sig.data(), &siglen,
                             msg.data(), msg.size(),
                             priv.data()) != 0)
        throw runtime_error("signing failed");

    sig.resize(siglen);
    return sig;
}

bool verify(const vector<uint8_t>& msg,
            const vector<uint8_t>& sig,
            const vector<uint8_t>& pub) {

    return crypto_sign_verify_detached(
        sig.data(), msg.data(), msg.size(),
        pub.data()
    ) == 0;
}

} // namespace crypto


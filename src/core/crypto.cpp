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
// ======================
// Simple Base64 encoder
// ======================
static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string crypto::toBase64(const std::vector<uint8_t>& data)
{
    std::string out;
    size_t i = 0;

    while (i < data.size()) {
        uint32_t a = i < data.size() ? data[i++] : 0;
        uint32_t b = i < data.size() ? data[i++] : 0;
        uint32_t c = i < data.size() ? data[i++] : 0;

        uint32_t triple = (a << 16) | (b << 8) | c;

        out.push_back(b64_table[(triple >> 18) & 0x3F]);
        out.push_back(b64_table[(triple >> 12) & 0x3F]);
        out.push_back(i > data.size()+1 ? '=' : b64_table[(triple >> 6) & 0x3F]);
        out.push_back(i > data.size()   ? '=' : b64_table[(triple >> 0) & 0x3F]);
    }
    return out;
}

// ======================
// Simple Base64 decoder
// ======================
static inline uint8_t b64_index(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 0;
}

std::vector<uint8_t> crypto::fromBase64(const std::string& s)
{
    size_t len = s.size();
    if (len % 4 != 0) return {};

    std::vector<uint8_t> out;
    out.reserve((len * 3) / 4);

    for (size_t i = 0; i < len; i += 4) {
        uint32_t v =
            (b64_index(s[i]) << 18) |
            (b64_index(s[i+1]) << 12) |
            ((s[i+2] == '=') ? 0 : (b64_index(s[i+2]) << 6)) |
            ((s[i+3] == '=') ? 0 : (b64_index(s[i+3])));

        out.push_back((v >> 16) & 0xFF);
        if (s[i+2] != '=') out.push_back((v >> 8) & 0xFF);
        if (s[i+3] != '=') out.push_back((v >> 0) & 0xFF);
    }
    return out;
}


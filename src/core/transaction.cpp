#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include "crypto/hash.hpp"
#include <sodium.h>
#include <iostream>
#include <iomanip>

using namespace std;

vector<uint8_t> Transaction::serialize() const {
    vector<uint8_t> data;
    data.insert(data.end(), senderPubkey.begin(), senderPubkey.end());
    for (int i = 0; i < 8; ++i)
        data.push_back((nonce >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i)
        data.push_back((fee >> (i * 8)) & 0xFF);
    data.insert(data.end(), payload.begin(), payload.end());
    return data;
}

void Transaction::sign(const vector<uint8_t>& priv) {
    const auto msg = serialize();
    signature = ::sign(msg, priv);
}

bool Transaction::verifySignature() const {
    const auto msg = serialize();
    if (senderPubkey.size() != crypto_sign_PUBLICKEYBYTES) {
        cerr << "[DEBUG] Invalid pubkey length: " << senderPubkey.size() << endl;
        return false;
    }
    if (signature.size() != crypto_sign_BYTES) {
        cerr << "[DEBUG] Invalid signature length: " << signature.size() << endl;
        return false;
    }
    return ::verify(msg, signature, senderPubkey);
}

array<uint8_t, 32> Transaction::hash() const {
    const auto bytes = serialize();
    return sha256(bytes);
}

void Transaction::deserialize(const vector<uint8_t>& data) {
    payload = data;
}

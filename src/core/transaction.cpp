#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include <sodium.h>
#include <iostream>

using namespace std;

vector<uint8_t> Transaction::serialize() const {
    vector<uint8_t> data;

    data.insert(data.end(), senderPubkey.begin(), senderPubkey.end());

    for (int i = 0; i < 8; i++)
        data.push_back((nonce >> (i * 8)) & 0xFF);

    for (int i = 0; i < 8; i++)
        data.push_back((fee >> (i * 8)) & 0xFF);

    data.insert(data.end(), payload.begin(), payload.end());
    return data;
}

void Transaction::sign(const vector<uint8_t>& priv) {
    const auto msg = serialize();
    signature = crypto::sign(msg, priv);   // <-- FIX
}

bool Transaction::verifySignature() const {
    const auto msg = serialize();

    if (senderPubkey.size() != crypto_sign_PUBLICKEYBYTES) {
        cerr << "[DEBUG] Invalid pubkey length: " << senderPubkey.size() << "\n";
        return false;
    }

    if (signature.size() != crypto_sign_BYTES) {
        cerr << "[DEBUG] Invalid signature length: " << signature.size() << "\n";
        return false;
    }

    return crypto::verify(msg, signature, senderPubkey);  // <-- FIX
}

array<uint8_t, 32> Transaction::hash() const {
    return crypto::sha256(serialize());
}

void Transaction::deserialize(const vector<uint8_t>& data) {
    payload = data;
}


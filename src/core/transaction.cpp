#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// -----------------------------------------------------------
// serialize() (wie vorher) ...
// -----------------------------------------------------------

std::vector<uint8_t> Transaction::serialize() const {
    json j = {
        {"senderPubkey", senderPubkey},
        {"payload",      payload},
        {"nonce",        nonce},
        {"fee",          fee}
    };
    std::string s = j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

std::array<uint8_t,32> Transaction::hash() const {
    return crypto::sha256(serialize());
}

void Transaction::sign(const std::vector<uint8_t>& priv) {
    signature = crypto::sign(payload, priv);
}

bool Transaction::verifySignature() const {
    if (senderPubkey.empty() || signature.empty())
        return false;
    return crypto::verify(payload, signature, senderPubkey);
}

// -----------------------------------------------------------
// ⭐ IMPLEMENTATION: Transaction::deserialize
// -----------------------------------------------------------
void Transaction::deserialize(const std::vector<uint8_t>& data)
{
    std::string s(data.begin(), data.end());
    json j = json::parse(s);

    senderPubkey = j.value("senderPubkey", std::vector<uint8_t>{});
    payload      = j.value("payload",      std::vector<uint8_t>{});
    nonce        = j.value("nonce",        0ull);
    fee          = j.value("fee",          0ull);

    // Signatur ist NICHT Bestandteil des serialisierten Hash-Bodies
    // → wird bei P2P separat übergeben
    if (j.contains("signature"))
        signature = j["signature"].get<std::vector<uint8_t>>();
    else
        signature.clear();
}


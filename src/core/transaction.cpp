#include "core/transaction.hpp"
#include "core/crypto.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// -----------------------------------------------------------
// serialize()
//  -> Body der Transaction OHNE Signatur
//  -> wird benutzt für Hash UND Signatur
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

// -----------------------------------------------------------
// Hash = sha256(serialize())
// -----------------------------------------------------------
std::array<uint8_t,32> Transaction::hash() const {
    return crypto::sha256(serialize());
}

// -----------------------------------------------------------
// Signiert den kompletten TX-Body (serialize()),
// NICHT nur die Payload.
// -----------------------------------------------------------
void Transaction::sign(const std::vector<uint8_t>& priv) {
    auto body = serialize();
    signature = crypto::sign(body, priv);
}

// -----------------------------------------------------------
// Verifiziert die Signatur über denselben Body wie beim Signen.
// -----------------------------------------------------------
bool Transaction::verifySignature() const {
    if (senderPubkey.empty() || signature.empty())
        return false;

    auto body = serialize();
    return crypto::verify(body, signature, senderPubkey);
}

// -----------------------------------------------------------
// deserialize()
//  -> Wird nur benutzt, wenn irgendwo der TX-Body als JSON
//     gespeichert/übertragen wird. Signatur kann optional sein.
// -----------------------------------------------------------
void Transaction::deserialize(const std::vector<uint8_t>& data)
{
    std::string s(data.begin(), data.end());
    json j = json::parse(s);

    senderPubkey = j.value("senderPubkey", std::vector<uint8_t>{});
    payload      = j.value("payload",      std::vector<uint8_t>{});
    nonce        = j.value("nonce",        0ull);
    fee          = j.value("fee",          0ull);

    // Signatur ist NICHT Bestandteil des Hash-Bodies,
    // kann aber optional im JSON mit drin sein.
    if (j.contains("signature"))
        signature = j["signature"].get<std::vector<uint8_t>>();
    else
        signature.clear();
}


#include "dnd/dndTx.hpp"
#include "dnd/serialization.hpp"
#include "core/crypto.hpp"
#include <sodium.h>
#include <ctime>

namespace dnd {

// ============================================================
// Spieler-Key erzeugen
// ============================================================

bool generatePlayerKeypair(std::vector<uint8_t>& pubOut,
                           std::vector<uint8_t>& privOut)
{
    pubOut.resize(crypto_sign_PUBLICKEYBYTES);
    privOut.resize(crypto_sign_SECRETKEYBYTES);

    crypto_sign_keypair(pubOut.data(), privOut.data());
    return true;
}

// ============================================================
// Signiere DnD Event
// ============================================================

void signDndEvent(DndEventTx& evt,
                  const std::vector<uint8_t>& privKey)
{
    // Set timestamp automatically
    evt.timestamp = time(nullptr);

    // Serialize DnD payload into JSON bytes
    auto payloadBytes = encodeDndTx(evt);

    // signature = sign(payload, privKey)
    evt.signature = crypto::sign(payloadBytes, privKey);
}

// ============================================================
// Prüfe Signatur
// ============================================================

bool verifyDndEventSignature(const DndEventTx& evt,
                             std::string& err)
{
    if (evt.senderPubKey.size() != crypto_sign_PUBLICKEYBYTES) {
        err = "Bad pubkey length";
        return false;
    }
    if (evt.signature.size() != crypto_sign_BYTES) {
        err = "Bad signature length";
        return false;
    }

    auto payloadBytes = encodeDndTx(evt);

    if (!crypto::verify(payloadBytes, evt.signature, evt.senderPubKey)) {
        err = "Signature invalid";
        return false;
    }

    return true;
}

} // namespace dnd


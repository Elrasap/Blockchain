#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "core/crypto.hpp"
#include "core/transaction.hpp"

#include <sodium.h>

#include <stdexcept>
#include <utility>

namespace dnd {

bool generatePlayerKeypair(std::vector<uint8_t>& pubOut,
                           std::vector<uint8_t>& privOut)
{
    auto kp = crypto::generateKeyPair();
    pubOut  = kp.publicKey;
    privOut = kp.privateKey;
    return true;
}

void signDndEvent(DndEventTx& evt,
                  const std::vector<uint8_t>& privKey)
{
    if (privKey.size() != crypto_sign_SECRETKEYBYTES)
        throw std::runtime_error("invalid Ed25519 private key size");

    evt.senderPubKey.resize(crypto_sign_PUBLICKEYBYTES);
    if (crypto_sign_ed25519_sk_to_pk(evt.senderPubKey.data(),
                                     privKey.data()) != 0)
        throw std::runtime_error("could not derive Ed25519 public key");

    Transaction tx;
    tx.payload = encodeDndTx(evt);
    tx.senderPubkey = evt.senderPubKey;
    tx.nonce = evt.transactionNonce;
    tx.sign(privKey);
    evt.signature = std::move(tx.signature);
}

bool verifyDndEventSignature(const DndEventTx& evt,
                             std::string& err)
{
    if (evt.senderPubKey.empty()) {
        err = "missing senderPubKey";
        return false;
    }
    if (evt.signature.empty()) {
        err = "missing signature";
        return false;
    }

    Transaction tx;
    tx.payload = encodeDndTx(evt);
    tx.senderPubkey = evt.senderPubKey;
    tx.signature = evt.signature;
    tx.nonce = evt.transactionNonce;
    if (!tx.verifySignature()) {
        err = "invalid canonical transaction signature";
        return false;
    }

    return true;
}

} // namespace dnd

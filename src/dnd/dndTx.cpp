#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "core/crypto.hpp"

namespace dnd {

bool generatePlayerKeypair(std::vector<uint8_t>& pubOut,
                           std::vector<uint8_t>& privOut)
{
    auto kp = crypto::generateKeyPair();
    pubOut  = kp.publicKey;
    privOut = kp.privateKey;
    return true;
}

// signiert den DnD-Event-Body (encodeDndTx) mit privKey
void signDndEvent(DndEventTx& evt,
                  const std::vector<uint8_t>& privKey)
{
    std::vector<uint8_t> body = encodeDndTx(evt);
    evt.signature = crypto::sign(body, privKey);
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

    std::vector<uint8_t> body = encodeDndTx(evt);
    if (!crypto::verify(body, evt.signature, evt.senderPubKey)) {
        err = "invalid DnD event signature";
        return false;
    }

    return true;
}

} // namespace dnd


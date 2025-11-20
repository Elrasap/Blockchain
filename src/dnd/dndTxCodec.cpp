#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTx.hpp"

#include <nlohmann/json.hpp>

namespace dnd {

using json = nlohmann::json;

// -------------------------------------------------------------
// Prüfen, ob Payload DnD JSON ist
// -------------------------------------------------------------
bool isDndPayload(const std::vector<uint8_t>& payload)
{
    if (payload.empty())
        return false;

    try {
        std::string s(payload.begin(), payload.end());
        auto j = json::parse(s);
        return j.contains("encounterId");
    } catch (...) {
        return false;
    }
}

// -------------------------------------------------------------
// Encode DnD TX → JSON → bytes
// WICHTIG: Dies ist der signierte Body → KEINE Signatur,
// KEIN senderPubKey im JSON.
// -------------------------------------------------------------
std::vector<uint8_t> encodeDndTx(const DndEventTx& tx)
{
    json j = {
        {"encounterId", tx.encounterId},
        {"actorType",   tx.actorType},
        {"actorId",     tx.actorId},
        {"targetType",  tx.targetType},
        {"targetId",    tx.targetId},
        {"roll",        tx.roll},
        {"damage",      tx.damage},
        {"hit",         tx.hit},
        {"note",        tx.note},
        {"timestamp",   tx.timestamp}
    };

    const std::string s = j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

// -------------------------------------------------------------
// Decode bytes → JSON → DndEventTx
// (Signatur & senderPubKey werden extern gesetzt)
// -------------------------------------------------------------
DndEventTx decodeDndTx(const std::vector<uint8_t>& buf)
{
    DndEventTx tx;

    std::string s(buf.begin(), buf.end());
    json j = json::parse(s);

    tx.encounterId = j.value("encounterId", "");
    tx.actorType   = j.value("actorType", 0);
    tx.actorId     = j.value("actorId", "");
    tx.targetType  = j.value("targetType", 0);
    tx.targetId    = j.value("targetId", "");

    tx.roll        = j.value("roll", 0);
    tx.damage      = j.value("damage", 0);
    tx.hit         = j.value("hit", false);
    tx.note        = j.value("note", "");

    tx.timestamp   = j.value("timestamp", 0ull);

    tx.senderPubKey.clear();
    tx.signature.clear();

    return tx;
}

} // namespace dnd


#pragma once
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "dnd/dndTx.hpp"

namespace dnd {

using json = nlohmann::json;

// ============================================================
// Encode DnD TX (JSON → bytes)
// ============================================================
inline std::vector<uint8_t> encodeDndTx(const DndEventTx& tx)
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
        {"timestamp",   tx.timestamp},
        {"senderPubKey", tx.senderPubKey},
        {"signature",    tx.signature}
    };

    const std::string s = j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

// ============================================================
// Decode bytes → DnD TX
// ============================================================
inline DndEventTx decodeDndTx(const std::vector<uint8_t>& buf)
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

    if (j.contains("senderPubKey"))
        tx.senderPubKey = j["senderPubKey"].get<std::vector<uint8_t>>();

    if (j.contains("signature"))
        tx.signature = j["signature"].get<std::vector<uint8_t>>();

    return tx;
}

} // namespace dnd


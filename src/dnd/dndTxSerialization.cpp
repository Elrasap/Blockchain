// src/dnd/dndTxSerialization.cpp
#include "dnd/dndTxSerialization.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTx.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dnd {

// ------------------------------------------------------------
// serializeDndTx – ruft einfach deinen Codec auf
// ------------------------------------------------------------
std::vector<uint8_t> serializeDndTx(const DndEventTx& evt)
{
    return encodeDndTx(evt);
}

// ------------------------------------------------------------
// deserializeDndTx – ruft deinen Codec auf
// ------------------------------------------------------------
DndEventTx deserializeDndTx(const std::vector<uint8_t>& buf)
{
    return decodeDndTx(buf);
}

// ------------------------------------------------------------
// Debug JSON (nur für Logging / Debug-Ausgaben)
// ------------------------------------------------------------
std::string dndTxToJson(const DndEventTx& evt)
{
    json j;
    j["encounterId"]  = evt.encounterId;

    j["actorId"]      = evt.actorId;
    j["actorType"]    = evt.actorType;

    j["targetId"]     = evt.targetId;
    j["targetType"]   = evt.targetType;

    j["roll"]         = evt.roll;
    j["damage"]       = evt.damage;
    j["hit"]          = evt.hit;
    j["note"]         = evt.note;

    j["timestamp"]    = evt.timestamp;

    j["senderPubKey"] = evt.senderPubKey;
    j["signature"]    = evt.signature;

    return j.dump(2);
}

} // namespace dnd


#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace dnd {

struct DndEventTx {
    std::string encounterId;

    std::string actorId;
    std::string targetId;

    int actorType  = 0;  // 0 = Character, 1 = Monster
    int targetType = 0;  // 0 = Character, 1 = Monster

    int  roll   = 0;
    int  damage = 0;
    bool hit    = false;
    std::string note;

    uint64_t timestamp = 0;

    // Signatur-Metadaten (nicht Teil des signierten Bodies)
    std::vector<uint8_t> senderPubKey;
    std::vector<uint8_t> signature;
};

// -------------------------------------------------------
// Signing API
// -------------------------------------------------------
bool generatePlayerKeypair(std::vector<uint8_t>& pubOut,
                           std::vector<uint8_t>& privOut);

void signDndEvent(DndEventTx& evt,
                  const std::vector<uint8_t>& privKey);

bool verifyDndEventSignature(const DndEventTx& evt,
                             std::string& err);

// -------------------------------------------------------
// JSON support (für State/Logging, NICHT fürs Signing)
// Der signierte Body enthält KEINE senderPubKey/signature.
// -------------------------------------------------------
using json = nlohmann::json;

inline void to_json(json& j, const DndEventTx& e)
{
    j = json{
        {"encounterId", e.encounterId},
        {"actorId",     e.actorId},
        {"actorType",   e.actorType},
        {"targetId",    e.targetId},
        {"targetType",  e.targetType},
        {"roll",        e.roll},
        {"damage",      e.damage},
        {"hit",         e.hit},
        {"note",        e.note},
        {"timestamp",   e.timestamp}
        // senderPubKey & signature sind NICHT Teil des Bodies
    };
}

inline void from_json(const json& j, DndEventTx& e)
{
    j.at("encounterId").get_to(e.encounterId);
    j.at("actorId").get_to(e.actorId);
    j.at("actorType").get_to(e.actorType);
    j.at("targetId").get_to(e.targetId);
    j.at("targetType").get_to(e.targetType);
    j.at("roll").get_to(e.roll);
    j.at("damage").get_to(e.damage);
    j.at("hit").get_to(e.hit);
    j.at("note").get_to(e.note);
    j.at("timestamp").get_to(e.timestamp);

    // Metadaten werden ggf. extern gesetzt
    e.senderPubKey.clear();
    e.signature.clear();
}

} // namespace dnd


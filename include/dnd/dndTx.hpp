#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>

#include <nlohmann/json.hpp>

namespace dnd {

// -------------------------------
// DnD-Event-Transaktion
// -------------------------------
struct DndEventTx {
    std::string encounterId;

    std::string actorId;
    std::string targetId;

    int actorType  = 0; // 0 = Character, 1 = Monster
    int targetType = 0; // 0 = Character, 1 = Monster

    int  roll   = 0;
    int  damage = 0;
    bool hit    = false;

    std::string note;

    uint64_t timestamp = 0;

    std::vector<uint8_t> senderPubKey;
    std::vector<uint8_t> signature;
};

// =====================================================
// JSON-Unterstützung für DndEventTx
// =====================================================

using json = nlohmann::json;

inline void to_json(json& j, const DndEventTx& e) {
    j = json{
        {"encounterId", e.encounterId},
        {"actorId",     e.actorId},
        {"targetId",    e.targetId},
        {"actorType",   e.actorType},
        {"targetType",  e.targetType},
        {"roll",        e.roll},
        {"damage",      e.damage},
        {"hit",         e.hit},
        {"note",        e.note},
        {"timestamp",   e.timestamp},
        {"senderPubKey", e.senderPubKey},
        {"signature",    e.signature}
    };
}

inline void from_json(const json& j, DndEventTx& e) {
    j.at("encounterId").get_to(e.encounterId);
    j.at("actorId").get_to(e.actorId);
    j.at("targetId").get_to(e.targetId);

    j.at("actorType").get_to(e.actorType);
    j.at("targetType").get_to(e.targetType);

    j.at("roll").get_to(e.roll);
    j.at("damage").get_to(e.damage);
    j.at("hit").get_to(e.hit);

    j.at("note").get_to(e.note);
    j.at("timestamp").get_to(e.timestamp);

    if (j.contains("senderPubKey")) {
        j.at("senderPubKey").get_to(e.senderPubKey);
    } else {
        e.senderPubKey.clear();
    }

    if (j.contains("signature")) {
        j.at("signature").get_to(e.signature);
    } else {
        e.signature.clear();
    }
}

// =====================================================
// SIGNING API – nur Deklarationen
// (Implementierung in src/dnd/dndTx.cpp oder ähnlich)
// =====================================================

bool generatePlayerKeypair(std::vector<uint8_t>& pubOut,
                           std::vector<uint8_t>& privOut);

void signDndEvent(DndEventTx& evt,
                  const std::vector<uint8_t>& privKey);

bool verifyDndEventSignature(const DndEventTx& evt,
                             std::string& err);

} // namespace dnd


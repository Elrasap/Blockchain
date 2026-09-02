#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace dnd {




enum class DndEventType : uint8_t {
    Unknown         = 0,
    CreateCharacter = 1,
    SpawnMonster    = 2,
    StartEncounter  = 3,
    Initiative      = 4,
    Hit             = 5,
    Damage          = 6,
    SkillCheck      = 7,
    EndEncounter    = 8
};

struct DndEventTx {
    std::string encounterId;

    std::string actorId;
    std::string targetId;

    int actorType  = 0;
    int targetType = 0;

    int  roll   = 0;
    int  damage = 0;
    bool hit    = false;
    std::string note;

    uint64_t timestamp = 0;


    DndEventType eventType = DndEventType::Unknown;

    // Set by the DM when a character is created. It is part of the
    // signed payload and becomes the character's controller key.
    std::vector<uint8_t> ownerPubKey;


    std::vector<uint8_t> senderPubKey;
    // Signature of the complete canonical Transaction envelope.
    std::vector<uint8_t> signature;

    // Envelope metadata used by HTTP clients. It is serialized by
    // Transaction, not by the DnD payload codec.
    uint64_t transactionNonce = 0;
};





bool generatePlayerKeypair(std::vector<uint8_t>& pubOut,
                           std::vector<uint8_t>& privOut);

void signDndEvent(DndEventTx& evt,
                  const std::vector<uint8_t>& privKey);

bool verifyDndEventSignature(const DndEventTx& evt,
                             std::string& err);




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
        {"timestamp",   e.timestamp},
        {"eventType",   static_cast<int>(e.eventType)},
        {"ownerPubKey", e.ownerPubKey},
        {"senderPubKey", e.senderPubKey},
        {"signature", e.signature},
        {"nonce", e.transactionNonce}

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

    int et = 0;
    if (j.contains("eventType"))
        et = j.at("eventType").get<int>();
    e.eventType = static_cast<DndEventType>(et);

    e.ownerPubKey = j.value("ownerPubKey", std::vector<uint8_t>{});
    e.senderPubKey = j.value("senderPubKey", std::vector<uint8_t>{});
    e.signature = j.value("signature", std::vector<uint8_t>{});
    e.transactionNonce = j.value("nonce", uint64_t{0});
}

}

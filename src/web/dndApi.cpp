#include "web/dndApi.hpp"
#include <ctime>
#include <iostream>

using namespace dnd;

// ======================================================
// Constructor
// ======================================================
DndApi::DndApi(Blockchain& chain,
               Mempool& mem,
               PeerManager* peers,
               DndTxValidator& validator,
               const std::vector<uint8_t>& dmPriv,
               const std::vector<uint8_t>& dmPub)
    : chain_(chain),
      mempool_(mem),
      peers_(peers),
      validator_(validator),
      dmPriv_(dmPriv),
      dmPub_(dmPub)
{}


// ======================================================
// JSON helpers
// ======================================================
httplib::Response DndApi::jsonError(const std::string& msg, int status)
{
    httplib::Response r;
    r.status = status;
    r.set_content(
        json{{"ok", false}, {"error", msg}}.dump(2),
        "application/json"
    );
    return r;
}

httplib::Response DndApi::jsonOK(const json& data)
{
    json out = data;
    out["ok"] = true;

    httplib::Response r;
    r.status = 200;
    r.set_content(out.dump(2), "application/json");
    return r;
}


// ======================================================
// JSON → DnD Event Mapping
// ======================================================
bool DndApi::parseJsonToEvent(const httplib::Request& req,
                              DndEventTx& evt,
                              std::string& errOut)
{
    json j;

    try {
        j = json::parse(req.body);
    } catch (...) {
        errOut = "invalid JSON";
        return false;
    }

    if (!j.contains("encounterId")) {
        errOut = "missing encounterId";
        return false;
    }
    if (!j.contains("actorId")) {
        errOut = "missing actorId";
        return false;
    }

    evt.encounterId = j.value("encounterId", "");
    evt.actorId     = j.value("actorId", "");
    evt.targetId    = j.value("targetId", "");

    evt.actorType   = j.value("actorType", 0);
    evt.targetType  = j.value("targetType", 0);

    evt.roll        = j.value("roll", 0);
    evt.damage      = j.value("damage", 0);
    evt.hit         = j.value("hit", false);
    evt.note        = j.value("note", "");

    evt.timestamp   = (uint64_t)time(nullptr);

    if (j.contains("senderPubKey"))
        evt.senderPubKey = j["senderPubKey"].get<std::vector<uint8_t>>();

    if (j.contains("signature"))
        evt.signature = j["signature"].get<std::vector<uint8_t>>();

    // Signing decision
    if (j.contains("privateKey")) {
        auto priv = j["privateKey"].get<std::vector<uint8_t>>();
        signDndEvent(evt, priv);
    }
    else if (evt.senderPubKey.empty()) {
        evt.senderPubKey = dmPub_;
        signDndEvent(evt, dmPriv_);
    }

    return true;
}


// ======================================================
// wrap event → TX → validate → mempool → broadcast
// ======================================================
bool DndApi::wrapAndInsert(const dnd::DndEventTx& evt,
                           std::string& errOut)
{
    // 1) Erst den DnD Event validieren
    if (!validator_.validate(evt, errOut)) {
        return false;
    }

    // 2) Event in Transaction einpacken
    Transaction tx;
    tx.payload      = encodeDndTx(evt);
    tx.senderPubkey = evt.senderPubKey;
    tx.signature    = evt.signature;

    // 3) Standard-Transaction-Validierung (Signatur etc.)
    if (!mempool_.addTransactionValidated(tx, errOut)) {
        return false;
    }

    // 4) Broadcast
    if (peers_) {
        peers_->broadcastTransaction(tx);
    }

    return true;
}

// ======================================================
// Install all endpoints
// ======================================================
void DndApi::install(httplib::Server& server)
{
    auto handler = [&](const httplib::Request& req,
                       httplib::Response& res,
                       const std::string& name)
    {
        DndEventTx evt;
        std::string err;

        if (!parseJsonToEvent(req, evt, err)) {
            res = jsonError(name + ": " + err);
            return;
        }

        if (!wrapAndInsert(evt, err)) {
            res = jsonError(name + ": " + err);
            return;
        }

        res = jsonOK({{"event", name}});
    };

    server.Post("/dnd/createCharacter",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "createCharacter");
                });

    server.Post("/dnd/spawnMonster",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "spawnMonster");
                });

    server.Post("/dnd/startEncounter",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "startEncounter");
                });

    server.Post("/dnd/initiative",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "initiative");
                });

    server.Post("/dnd/hit",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "hit");
                });

    server.Post("/dnd/damage",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "damage");
                });

    server.Post("/dnd/skillCheck",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "skillCheck");
                });

    server.Post("/dnd/endEncounter",
                [&](const httplib::Request& req, httplib::Response& res){
                    handler(req, res, "endEncounter");
                });
}


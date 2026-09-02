#pragma once

#include <string>
#include <vector>

#include "dnd/dndTx.hpp"

class Blockchain;
class Mempool;
class PeerManager;

namespace httplib {
class Server;
class Request;
class Response;
} // namespace httplib

namespace dnd {

class DndApi {
public:
    DndApi(Blockchain& chain,
           Mempool& mempool,
           PeerManager* peers,
           const std::vector<uint8_t>& dmPriv,
           const std::vector<uint8_t>& dmPub);

    // Registriert alle HTTP-Routen (/dnd/…)
    void install(httplib::Server& server);

private:
    Blockchain&          chain_;
    Mempool&             mempool_;
    PeerManager*         peers_;
    std::vector<uint8_t> dmPriv_;
    std::vector<uint8_t> dmPub_;

    // Basis-JSON-Antworten
    static httplib::Response jsonError(const std::string& msg, int status);
    static httplib::Response jsonOK(const nlohmann::json& data);

    // Request → DndEventTx parsen (KEINE Eventsig mehr, nur senderPubKey)
    bool parseJsonToEvent(const httplib::Request& req,
                          DndEventTx& evt,
                          std::string& errOut);

    // DnD-Event als Transaction wrappen, TX signieren, validieren, in Mempool legen, broadcasten
    bool wrapAndInsert(const DndEventTx& evtInput,
                       bool allowDmSigning,
                       std::string& errOut);

    // GET /dnd/history/<encId>
    httplib::Response getEncounterHistory(const std::string& encId);

    // GET /dnd/state
    httplib::Response getState();
};

} // namespace dnd

#pragma once

#include <string>
#include <vector>

#include "dnd/dndTx.hpp"
#include "dnd/dndTxValidator.hpp"

class Blockchain;
class Mempool;
class PeerManager;

namespace httplib {
class Server;
class Request;
class Response;
}

namespace dnd {

class DndApi {
public:
    DndApi(Blockchain& chain,
           Mempool& mempool,
           PeerManager* peers,
           dnd::DndTxValidator& validator,
           const std::vector<uint8_t>& dmPriv,
           const std::vector<uint8_t>& dmPub);

    void install(httplib::Server& server);

private:
    Blockchain&          chain_;
    Mempool&             mempool_;
    PeerManager*         peers_;
    dnd::DndTxValidator& validator_;
    std::vector<uint8_t> dmPriv_;
    std::vector<uint8_t> dmPub_;

    static httplib::Response jsonError(const std::string& msg, int status);
    static httplib::Response jsonOK(const nlohmann::json& data);

    bool parseJsonToEvent(const httplib::Request& req,
                          DndEventTx& evt,
                          std::string& errOut);

    bool wrapAndInsert(const DndEventTx& evtInput,
                       std::string& errOut);

    httplib::Response getEncounterHistory(const std::string& encId);

    httplib::Response getState();
};

} // namespace dnd


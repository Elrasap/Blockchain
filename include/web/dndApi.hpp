#pragma once
#include <string>
#include <vector>

#include "thirdparty/httplib.h"
#include "core/blockchain.hpp"
#include "core/mempool.hpp"
#include "network/peerManager.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxValidator.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dnd {

class DndApi {
public:
    DndApi(Blockchain& chain,
           Mempool& mempool,
           PeerManager* peers,
           dnd::DndTxValidator& validator,
           const std::vector<uint8_t>& dmPrivKey,
           const std::vector<uint8_t>& dmPubKey);

    void install(httplib::Server& server);

private:
    Blockchain& chain_;
    Mempool& mempool_;
    PeerManager* peers_;
    dnd::DndTxValidator& validator_;
    std::vector<uint8_t> dmPriv_;
    std::vector<uint8_t> dmPub_;

    static httplib::Response jsonError(const std::string& msg, int status = 400);
    static httplib::Response jsonOK(const json& data = json::object());

    bool parseJsonToEvent(const httplib::Request& req,
                          dnd::DndEventTx& evt,
                          std::string& errOut);

    bool wrapAndInsert(const dnd::DndEventTx& evt,
                       std::string& errOut);

    // Neu: History-Antwort
    httplib::Response getEncounterHistory(const std::string& encId);
};

} // namespace dnd


#pragma once
#include <string>
#include <vector>

#include "thirdparty/httplib.h"
#include <nlohmann/json.hpp>

#include "core/mempool.hpp"
#include "core/transaction.hpp"
#include "network/peerManager.hpp"
#include "dnd/dndTx.hpp"
#include "dnd/dndTxCodec.hpp"
#include "dnd/dndTxValidator.hpp"

class Blockchain;

using json = nlohmann::json;

namespace dnd {

class DndApi {
public:
    DndApi(Blockchain& chain,
           Mempool& mempool,
           PeerManager* peers,
           DndTxValidator& validator,
           const std::vector<uint8_t>& dmPriv,
           const std::vector<uint8_t>& dmPub);

    void install(httplib::Server& svr);

private:
    Blockchain& chain_;
    Mempool& mempool_;
    PeerManager* peers_;
    DndTxValidator& validator_;

    std::vector<uint8_t> dmPriv_;
    std::vector<uint8_t> dmPub_;

    // helpers
    static httplib::Response jsonOK(const json& obj = json::object());
    static httplib::Response jsonError(const std::string& msg, int status = 400);

    bool parseJsonToEvent(const httplib::Request& req,
                          DndEventTx& evt,
                          std::string& errOut);

    bool wrapAndInsert(const DndEventTx& evt,
                       std::string& errOut);
};

} // namespace dnd


#pragma once
#pragma once
#include "core/blockchain.hpp"
#include "core/mempool.hpp"

namespace web {

class DndApi {
public:
    DndApi(Blockchain& chain, Mempool& mempool);

    // Registers routes in the provided httplib::Server
    void registerRoutes(httplib::Server& server);

private:
    Blockchain& chain_;
    Mempool& mempool_;
};

} // namespace web


#include <vector>
#include <cstdint>

class Blockchain;
class Mempool;
namespace httplib { class Server; }

namespace web {

class DndApi {
public:
    DndApi(httplib::Server& server,
           Blockchain& chain,
           Mempool& mempool,
           const std::vector<uint8_t>& dmPriv,
           const std::vector<uint8_t>& dmPub);

private:
    void registerEndpoints(httplib::Server& server);

    Blockchain& chain_;
    Mempool& mempool_;

    std::vector<uint8_t> dmPrivKey_;
    std::vector<uint8_t> dmPubKey_;
};

} // namespace web


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


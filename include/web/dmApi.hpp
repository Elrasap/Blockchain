#pragma once
#include "core/blockchain.hpp"
#include "core/blockBuilder.hpp"
#include "core/mempool.hpp"
#include <httplib.h>

namespace web {

class DmApi {
public:
    DmApi(Blockchain& chain,
          Mempool& mempool,
          const std::vector<uint8_t>& dmPrivKey,
          const std::vector<uint8_t>& dmPubKey);

    void registerRoutes(httplib::Server& server);

private:
    Blockchain& chain_;
    Mempool& mempool_;
    std::vector<uint8_t> dmPriv_;
    std::vector<uint8_t> dmPub_;
};

} // namespace web


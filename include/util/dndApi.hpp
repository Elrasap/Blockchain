#pragma once

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


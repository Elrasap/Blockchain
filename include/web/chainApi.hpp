#pragma once
#include <thirdparty/httplib.h>
#include "core/blockchain.hpp"
#include "dnd/dndState.hpp"

namespace web {

class ChainApi {
public:
    ChainApi(Blockchain& chain, dnd::DndState& dndState);

    void registerRoutes(httplib::Server& server);

private:
    Blockchain& chain_;
    dnd::DndState& dndState_;
};

} // namespace web


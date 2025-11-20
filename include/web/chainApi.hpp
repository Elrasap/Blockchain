#pragma once
#include <thirdparty/httplib.h>
#include "core/blockchain.hpp"

class ChainApi {
public:
    ChainApi(Blockchain& chain);
    void bind(httplib::Server& server);

private:
    Blockchain& chain_;
};


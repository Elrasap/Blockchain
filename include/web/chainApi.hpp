#pragma once
#include "thirdparty/httplib.h"
#include "core/blockchain.hpp"

class ChainApi {
public:
    explicit ChainApi(Blockchain& chain);

    void bind(httplib::Server& svr);

private:
    Blockchain& chain_;

    std::string hashToHex(const std::array<uint8_t, 32>& h) const;
};


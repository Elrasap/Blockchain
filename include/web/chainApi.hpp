#pragma once

#include <nlohmann/json.hpp>
#include "thirdparty/httplib.h"

class Blockchain;
class PeerManager;

class ChainApi {
public:
    ChainApi(Blockchain& chain, PeerManager* peers);

    void bind(httplib::Server& server);

private:
    Blockchain& chain_;
    PeerManager* peers_;

    static httplib::Response jsonOK(const nlohmann::json& j);
    static httplib::Response jsonErr(const std::string& msg, int status);
};


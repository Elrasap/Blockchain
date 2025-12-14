#pragma once
#include <string>
#include "types.hpp"

class Discovery
{
public:
    void loadSeenList();
    void sendHello();
    void handleHello();
    void peerRefreshLoop();
    void banPeer(B32 peerId,const std::string& reason);
};


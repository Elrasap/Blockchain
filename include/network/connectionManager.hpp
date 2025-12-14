#pragma once
#include <string>
#include "types.hpp"

class ConnectionManager
{
public:
    void start();
    void onMessage(const B32 peer, const std::string& message);
    void sendToAll(const std::string& message);
    void disconnect(const B32 peer);
    void heartbeatLoop();
};

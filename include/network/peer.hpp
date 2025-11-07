#pragma once
#include <string>
#include "types.hpp"

class Peer
{
public:
    B32 peerID;
    std::string address;
    string status;
    uint64_t lastSeen;
    std::sting capabilities;

    void updateStatus();
    void recordLatency();
    void markUnresponsive();
};

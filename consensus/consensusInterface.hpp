
#pragma once
#include <string>
#include "types.hpp"

class ConsensusInterface
{
public:
    void start();
    void proposeBlock(const Block& block);
    void onReceiveMessage(const B32 peer, const std::string& message);
    void getRole();
    void commitIndex();
};

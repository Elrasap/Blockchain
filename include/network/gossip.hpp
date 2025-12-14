#pragma once
#include <string>
#include "types.hpp"

class Gossip
{
public:
    void broadcastTx(const Transaction& tx);
    void broacastBlock(const Block& block);
    void requestMissing();
    void handleInv(const string message);
    void rateLimit(const B32 peerId);
};

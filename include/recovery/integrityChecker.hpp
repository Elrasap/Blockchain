#pragma once

class IntegrityChecker {
public:
    bool verifyWALChecksum(const WAL& w)
    bool verifyBlockChain(const BlockStore& bs);
    bool verifyStateRoot(const State& s);
};

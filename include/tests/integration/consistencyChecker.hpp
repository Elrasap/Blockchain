#pragma once
#include <cstdint>

class ConsistencyChecker {
public:
    bool compareHeads();
    bool compareStateRoots();
    bool detectFork();
};


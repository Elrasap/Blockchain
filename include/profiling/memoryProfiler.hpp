#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <sys/time.h>
#include <sys/resource.h>

#include <unistd.h>

#include <execinfo.h>

class memoryProfiler {
public:
    void startSnapshot();
    void endSnapshot();
    void compareSnapshots();
};

#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "cpuProfiler.h"

namespace cpuProfiler {
public:
    void start(uint64_t durationMsi);
    void collectSamples();
    void exportFlamegraph(const std::string& path);
};

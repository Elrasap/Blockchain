#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

class ResourceMonitor {
public:
    double cpuUsage();
    double memoryUsage();
    double diskIOps();
    double networkThroughput();
};

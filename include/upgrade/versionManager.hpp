#pragma once
#include <string>
#include <vector>
#include <memory>

class VersionManager {
public:
    uint32_t getCurrentProtocolVersion();
    bool isCompatible(uint32_t peerVersion);
    void registerFeature(const std::string& name, uint32_t minVersion);

};

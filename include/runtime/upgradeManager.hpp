#pragma once
#include <string>

class UpgradeManager {
public:
    explicit UpgradeManager(const std::string& releaseDir);
    bool checkForNewVersion();
    bool downloadNewVersion();
    bool applyUpgrade();
private:
    std::string dir;
    std::string currentVersion;
    std::string targetVersion;
};


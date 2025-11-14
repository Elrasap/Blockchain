#include "runtime/upgradeManager.hpp"
#include <fstream>
#include <iostream>

UpgradeManager::UpgradeManager(const std::string& releaseDir)
    : dir(releaseDir), currentVersion("1.9.0"), targetVersion("") {}

bool UpgradeManager::checkForNewVersion() {
    std::ifstream in(dir + "/next_version.txt");
    if (!in) return false;
    std::getline(in, targetVersion);
    return !targetVersion.empty() && targetVersion != currentVersion;
}

bool UpgradeManager::downloadNewVersion() {
    std::ofstream out(dir + "/blockchain_node_" + targetVersion + ".bin");
    out << "NEW_VERSION_BINARY";
    return true;
}

bool UpgradeManager::applyUpgrade() {
    if (targetVersion.empty()) return false;
    currentVersion = targetVersion;
    std::cout << "[UpgradeManager] upgraded to version " << currentVersion << "\n";
    return true;
}


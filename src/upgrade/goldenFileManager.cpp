#include "upgrade/golden_file_manager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

GoldenFileManager::GoldenFileManager(const std::string& path)
    : base_path(path)
{
    std::filesystem::create_directories(base_path);
}

void GoldenFileManager::writeReference(const std::string& version,
                                       const std::array<uint8_t,32>& stateRoot,
                                       const std::string& schemaHash)
{
    std::ostringstream hex;
    for (auto b : stateRoot) {
        constexpr char map[] = "0123456789abcdef";
        hex << map[(b >> 4) & 0x0F] << map[b & 0x0F];
    }

    std::string file = base_path + "/golden_" + version + ".ref";
    std::ofstream out(file, std::ios::trunc);
    out << "state_root=" << hex.str() << "\n";
    out << "schema=" << schemaHash << "\n";
}

std::map<std::string,std::string> GoldenFileManager::readReference(const std::string& version)
{
    std::map<std::string,std::string> m;
    std::string file = base_path + "/golden_" + version + ".ref";
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 1);
        m[k] = v;
    }
    return m;
}

std::vector<std::string> GoldenFileManager::listVersions() const
{
    std::vector<std::string> v;
    for (auto& p : std::filesystem::directory_iterator(base_path)) {
        if (!p.is_regular_file()) continue;
        auto path = p.path().filename().string();
        if (path.rfind("golden_", 0) == 0 && path.size() > 12 && path.ends_with(".ref")) {
            std::string ver = path.substr(7, path.size() - 11);
            v.push_back(ver);
        }
    }
    return v;
}

bool GoldenFileManager::exists(const std::string& version) const
{
    std::string file = base_path + "/golden_" + version + ".ref";
    return std::filesystem::exists(file);
}


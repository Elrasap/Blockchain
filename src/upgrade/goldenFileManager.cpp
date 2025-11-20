#include "upgrade/goldenFileManager.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;


// ------------------------------------------------------------
// Konstruktor - Ordner erstellen
// ------------------------------------------------------------
GoldenFileManager::GoldenFileManager(const std::string& path)
    : base_path(path)
{
    fs::create_directories(base_path);
}


// ------------------------------------------------------------
// exists(version)
// ------------------------------------------------------------
bool GoldenFileManager::exists(const std::string& version) const
{
    std::string file = base_path + "/golden_" + version + ".ref";
    return fs::exists(file);
}


// ------------------------------------------------------------
// writeReference
// ------------------------------------------------------------
void GoldenFileManager::writeReference(const std::string& version,
                                       const std::array<uint8_t,32>& stateRoot,
                                       const std::string& schemaHash)
{
    static const char* M = "0123456789abcdef";

    // Schneller: Hex-String direkt vorbereiten
    std::string hex;
    hex.reserve(64);

    for (uint8_t b : stateRoot) {
        hex.push_back(M[(b >> 4) & 0x0F]);
        hex.push_back(M[b & 0x0F]);
    }

    std::string file = base_path + "/golden_" + version + ".ref";
    std::ofstream out(file, std::ios::trunc);

    out << "state_root=" << hex << "\n";
    out << "schema=" << schemaHash << "\n";
}



// ------------------------------------------------------------
// readReference
// ------------------------------------------------------------
std::map<std::string,std::string>
GoldenFileManager::readReference(const std::string& version)
{
    std::map<std::string,std::string> m;
    std::string file = base_path + "/golden_" + version + ".ref";

    std::ifstream in(file);
    if (!in) return m;

    std::string line;
    while (std::getline(in, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        m[key] = val;
    }

    return m;
}


// ------------------------------------------------------------
// listVersions
// ------------------------------------------------------------
std::vector<std::string> GoldenFileManager::listVersions() const
{
    std::vector<std::string> v;

    if (!fs::exists(base_path))
        return v;

    for (auto& p : fs::directory_iterator(base_path)) {
        if (!p.is_regular_file()) continue;

        const std::string name = p.path().filename().string();

        // "golden_XXXX.ref"
        if (name.size() > 12 &&
            name.rfind("golden_", 0) == 0 &&
            name.substr(name.size() - 4) == ".ref")
        {
            // Extrahiere "XXXX" (zwischen golden_ und .ref)
            std::string ver = name.substr(7, name.size() - 11);
            v.push_back(ver);
        }
    }

    return v;
}


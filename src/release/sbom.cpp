#include "release/sbom.hpp"
#include <fstream>

bool Sbom::writeYaml(const std::string& path,
                     const std::string& artifactName,
                     const std::string& version,
                     const std::string& sha256hex,
                     const std::vector<std::pair<std::string,std::string>>& deps) {
    std::ofstream out(path);
    if (!out) return false;
    out << "sbomVersion: 1\n";
    out << "artifact:\n";
    out << "  name: " << artifactName << "\n";
    out << "  version: " << version << "\n";
    out << "  sha256: " << sha256hex << "\n";
    out << "dependencies:\n";
    for (auto& d : deps) out << "  - name: " << d.first << "\n    version: " << d.second << "\n";
    return true;
}


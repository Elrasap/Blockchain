#pragma once
#include <string>
#include <vector>
#include <utility>

class Sbom {
public:
    static bool writeYaml(const std::string& path,
                          const std::string& artifactName,
                          const std::string& version,
                          const std::string& sha256hex,
                          const std::vector<std::pair<std::string,std::string>>& deps);
};


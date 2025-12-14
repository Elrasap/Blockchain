#pragma once
#include <string>
#include <array>
#include <map>
#include <vector>
#include <cstdint>

class GoldenFileManager {
public:
    explicit GoldenFileManager(const std::string& path);

    bool exists(const std::string& version) const;

    void writeReference(const std::string& version,
                        const std::array<uint8_t,32>& stateRoot,
                        const std::string& schemaHash);

    std::map<std::string,std::string>
    readReference(const std::string& version);

    std::vector<std::string>
    listVersions() const;

private:
    std::string base_path;
};


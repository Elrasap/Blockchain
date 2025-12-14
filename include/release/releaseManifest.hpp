#pragma once
#include <string>

class ReleaseManifest {
public:
    static bool generate(const std::string& binaryPath,
                         const std::string& sbomPath,
                         const std::string& attestPath,
                         const std::string& keyFile,
                         const std::string& outputPath);
};


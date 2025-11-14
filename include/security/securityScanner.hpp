#pragma once
#include <string>
#include <vector>
#include <map>

struct ScanResult {
    bool checksumValid;
    bool signatureValid;
    bool sbomValid;
    bool attestationValid;
    std::vector<std::string> vulnerabilities;
};


class SecurityScanner {
public:
    SecurityScanner(const std::string& baseDir);

    // ADD THESE (match .cpp exactly)
    std::string findFile(const std::string& suffix) const;

    bool checkAttestation(const std::string& attestPath,
                          std::string& attestSha,
                          std::string& pubHex);

    bool checkChecksum(std::string& outSha,
                       std::string& sbomSha);

    bool checkSignature(const std::string& binPath,
                        const std::string& sigPath,
                        const std::string& pubHex);

    bool checkSbom(const std::string& sbomPath,
                   std::vector<std::string>& deps,
                   std::vector<std::string>& vulns);

    // existing API (if you still want them)
    bool runSimple();
    ScanResult run();

private:
    std::string dir;
};


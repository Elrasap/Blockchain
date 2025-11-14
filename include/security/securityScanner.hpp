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
    explicit SecurityScanner(const std::string& releaseDir);
    ScanResult run();
private:
    std::string dir;
    bool checkChecksum();
    bool checkSignature();
    bool checkSbom();
    bool checkAttestation();
    std::vector<std::string> scanDependencies();
};


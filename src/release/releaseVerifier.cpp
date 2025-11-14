#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

class ReleaseVerifier {
public:
    explicit ReleaseVerifier(const std::string& releaseDir);
    bool verifyIntegrity();
    bool verifySignature();
    bool verifySbom();
    bool verifyAttestation();
    bool verifyReproducibility(const std::string& rebuiltPath);
    void summarize() const;
private:
    std::string dir;
    std::string binPath;
    std::string sigPath;
    std::string sbomPath;
    std::string attestPath;
    std::string sha256Hex;
    bool integrityOk = false;
    bool signatureOk = false;
    bool sbomOk = false;
    bool attestOk = false;
    bool reproOk = false;
};


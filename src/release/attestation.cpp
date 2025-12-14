#include "release/attestation.hpp"
#include <fstream>

bool writeAttestationJson(const std::string& path,
                          const std::string& artifactName,
                          const std::string& sha256hex,
                          const std::string& signatureHex,
                          const std::string& pubkeyHex,
                          const std::string& builder) {
    std::ofstream out(path);
    if (!out) return false;
    out << "{";
    out << "\"type\":\"attestation\",\"artifact\":\"" << artifactName << "\",";
    out << "\"sha256\":\"" << sha256hex << "\",";
    out << "\"signature\":\"" << signatureHex << "\",";
    out << "\"pubkey\":\"" << pubkeyHex << "\",";
    out << "\"builder\":\"" << builder << "\"";
    out << "}";
    return true;
}


#pragma once
#include <string>

bool writeAttestationJson(const std::string& path,
                          const std::string& artifactName,
                          const std::string& sha256hex,
                          const std::string& signatureHex,
                          const std::string& pubkeyHex,
                          const std::string& builder);


#include "release/verifier.hpp"
#include "release/checksummer.hpp"
#include "release/signer.hpp"
#include "core/crypto.hpp"

bool verifyChecksumHex(const std::string& path, const std::string& expectedHex) {
    return fileSha256Hex(path) == expectedHex;
}

bool verifySignatureOverFile(const std::string& path,
                             const std::vector<uint8_t>& signature,
                             const std::vector<uint8_t>& pubkey) {
    auto d = fileSha256(path);
    std::vector<uint8_t> msg(d.begin(), d.end());
    return crypto::verify(msg, signature, pubkey);
}


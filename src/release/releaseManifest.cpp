#include "release/release_manifest.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <sodium.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

static std::string sha256File(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if(!f) throw std::runtime_error("Cannot open file: " + path);
    std::vector<unsigned char> buf(8192);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    while(f) {
        f.read(reinterpret_cast<char*>(buf.data()), buf.size());
        SHA256_Update(&ctx, buf.data(), f.gcount());
    }
    SHA256_Final(hash, &ctx);
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for(auto c: hash) ss << std::setw(2) << (int)c;
    return ss.str();
}

bool ReleaseManifest::generate(const std::string& binaryPath,
                               const std::string& sbomPath,
                               const std::string& attestPath,
                               const std::string& keyFile,
                               const std::string& outputPath) {
    if (sodium_init() < 0) {
        std::cerr << "libsodium init failed\n";
        return false;
    }

    std::string hash = sha256File(binaryPath);
    std::string sbomHash = fs::exists(sbomPath) ? sha256File(sbomPath) : "";
    std::string attestHash = fs::exists(attestPath) ? sha256File(attestPath) : "";

    json manifest;
    manifest["version"] = "1.0";
    manifest["timestamp"] = std::time(nullptr);
    manifest["binary"] = binaryPath;
    manifest["binary_sha256"] = hash;
    manifest["sbom"] = sbomPath;
    manifest["sbom_sha256"] = sbomHash;
    manifest["attestation"] = attestPath;
    manifest["attestation_sha256"] = attestHash;

    // read private key
    std::ifstream keyIn(keyFile, std::ios::binary);
    if (!keyIn) {
        std::cerr << "Missing Ed25519 key file\n";
        return false;
    }
    std::vector<unsigned char> sk(crypto_sign_SECRETKEYBYTES);
    keyIn.read(reinterpret_cast<char*>(sk.data()), sk.size());
    std::string msg = manifest.dump();

    // sign
    std::vector<unsigned char> sig(crypto_sign_BYTES);
    crypto_sign_detached(sig.data(), nullptr,
                         reinterpret_cast<const unsigned char*>(msg.data()),
                         msg.size(), sk.data());

    std::ostringstream sigHex;
    sigHex << std::hex << std::setfill('0');
    for(auto b : sig) sigHex << std::setw(2) << (int)b;

    manifest["signature"] = sigHex.str();

    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Cannot write manifest: " << outputPath << "\n";
        return false;
    }
    out << manifest.dump(2);
    std::cout << "[ReleaseManifest] Written: " << outputPath << "\n";
    return true;
}


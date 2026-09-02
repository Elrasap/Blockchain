#include "core/dmKeyManager.hpp"
#include "core/crypto.hpp"

#include <sodium.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

constexpr std::array<char, 8> KEY_MAGIC{'D', 'N', 'D', 'K', 'E', 'Y', '0', '1'};

bool readExistingKey(const std::filesystem::path& path, DmKeyPair& out)
{
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(path))) {
        std::cerr << "[DmKeyManager] Refusing symlinked key file\n";
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    std::array<char, KEY_MAGIC.size()> magic{};
    input.read(magic.data(), magic.size());
    if (!input || magic != KEY_MAGIC) {
        std::cerr << "[DmKeyManager] Invalid key file format\n";
        return false;
    }

    out.publicKey.resize(crypto_sign_PUBLICKEYBYTES);
    out.privateKey.resize(crypto_sign_SECRETKEYBYTES);
    input.read(reinterpret_cast<char*>(out.publicKey.data()), out.publicKey.size());
    input.read(reinterpret_cast<char*>(out.privateKey.data()), out.privateKey.size());
    if (!input || input.peek() != std::ifstream::traits_type::eof()) {
        std::cerr << "[DmKeyManager] Truncated or oversized key file\n";
        return false;
    }

    std::array<uint8_t, crypto_sign_PUBLICKEYBYTES> derived{};
    if (crypto_sign_ed25519_sk_to_pk(derived.data(), out.privateKey.data()) != 0 ||
        !std::equal(derived.begin(), derived.end(), out.publicKey.begin())) {
        std::cerr << "[DmKeyManager] Public/private key mismatch\n";
        return false;
    }

    ::chmod(path.c_str(), S_IRUSR | S_IWUSR);
    return true;
}

} // namespace

bool loadOrCreateDmKey(const std::string& pathString, DmKeyPair& out)
{
    const std::filesystem::path path(pathString);
    std::error_code error;
    if (std::filesystem::exists(path, error))
        return !error && readExistingKey(path, out);

    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            std::cerr << "[DmKeyManager] Could not create key directory: "
                      << error.message() << "\n";
            return false;
        }
        ::chmod(parent.c_str(), S_IRWXU);
    }

    try {
        auto generated = crypto::generateKeyPair();
        out.publicKey = std::move(generated.publicKey);
        out.privateKey = std::move(generated.privateKey);
    } catch (const std::exception& ex) {
        std::cerr << "[DmKeyManager] Key generation failed: "
                  << ex.what() << "\n";
        return false;
    }

    const std::filesystem::path temporary =
        path.string() + ".tmp." + std::to_string(::getpid());
    {
        std::ofstream output(temporary,
                             std::ios::binary | std::ios::trunc);
        if (!output) return false;
        output.write(KEY_MAGIC.data(), KEY_MAGIC.size());
        output.write(reinterpret_cast<const char*>(out.publicKey.data()),
                     out.publicKey.size());
        output.write(reinterpret_cast<const char*>(out.privateKey.data()),
                     out.privateKey.size());
        output.close();
        if (!output) {
            std::filesystem::remove(temporary, error);
            return false;
        }
    }

    ::chmod(temporary.c_str(), S_IRUSR | S_IWUSR);
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::cerr << "[DmKeyManager] Could not install key file: "
                  << error.message() << "\n";
        std::filesystem::remove(temporary, error);
        return false;
    }

    std::cout << "[DmKeyManager] Generated DM key at " << path << "\n";
    return true;
}

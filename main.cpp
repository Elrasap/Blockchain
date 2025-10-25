#include "crypto/sha256Hash.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>

int main() {
    Sha256Hash hasher;
    std::vector<uint8_t> data = {'h','e','l','l','o'};
    auto digest = hasher.hash(data);

    std::cout << hasher.name() << " digest: ";
    for (auto byte : digest)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    std::cout << std::endl;
}


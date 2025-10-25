#include "crypto/sha256Hash.hpp"
#include "crypto/types.hpp"
#include <iostream>
#include <vector>
#include <iomanip>

int main() {
    SHA256Hash hasher;
    std::vector<uint8_t> input = {1, 2, 3};
    auto digest = hasher.hash(input);

    std::cout << hasher.name() << " (" << hasher.digestSize() << " bytes)" << std::endl;
    std::cout << "Digest preview: ";
    for (size_t i = 0; i < std::min<size_t>(digest.size(), 8); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(digest[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

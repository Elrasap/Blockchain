#include "tests/testFramework.hpp"
#include <iostream>

int main() {
    auto& tests = getTestRegistry();
    std::cout << "[TEST] Registered " << tests.size() << " test cases.\n";

    int failed = 0;
    for (auto& t : tests) {
        try {
            std::cout << "[RUN] " << t.name << " ... ";
            t.fn();
            std::cout << "OK\n";
        } catch (const std::exception& ex) {
            std::cout << "FAILED: " << ex.what() << "\n";
            ++failed;
        } catch (...) {
            std::cout << "FAILED: unknown error\n";
            ++failed;
        }
    }

    std::cout << "[TEST] Done. Failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}


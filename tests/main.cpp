#include <iostream>
#include <vector>
#include <functional>

int main()
{
    std::cout << "[TEST] Registered " << TestRegistry::tests().size()
              << " test cases.\n";

    int fails = 0;

    for (auto& t : TestRegistry::tests()) {
        std::cout << "[RUN] " << t.name << " ... ";
        try {
            t.fn();
            std::cout << "OK\n";
        } catch (...) {
            std::cout << "FAILED\n";
            fails++;
        }
    }

    std::cout << "[TEST] Done. Failed: " << fails << "\n";
    return fails == 0 ? 0 : 1;
}


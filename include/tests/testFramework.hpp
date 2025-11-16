#pragma once
#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <stdexcept>

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& getTestRegistry() {
    static std::vector<TestCase> registry;
    return registry;
}

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> fn) {
        getTestRegistry().push_back({name, std::move(fn)});
    }
};

#define TEST_CASE(name)                                      \
    void name();                                             \
    static TestRegistrar _reg_##name(#name, name);           \
    void name()

#define ASSERT_TRUE(expr)                                                    \
    do {                                                                     \
        if (!(expr)) {                                                       \
            std::cerr << "[ASSERT_TRUE FAILED] " << #expr                    \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n";      \
            throw std::runtime_error("ASSERT_TRUE failed");                  \
        }                                                                    \
    } while (0)

#define ASSERT_EQ(a, b)                                                      \
    do {                                                                     \
        auto _va = (a);                                                      \
        auto _vb = (b);                                                      \
        if (!(_va == _vb)) {                                                 \
            std::cerr << "[ASSERT_EQ FAILED] " << #a << " == " << #b         \
                      << " at " << __FILE__ << ":" << __LINE__               \
                      << " (got " << _va << " vs " << _vb << ")\n";          \
            throw std::runtime_error("ASSERT_EQ failed");                    \
        }                                                                    \
    } while (0)

#define ASSERT_NE(a, b)                                                      \
    do {                                                                     \
        auto _va = (a);                                                      \
        auto _vb = (b);                                                      \
        if (_va == _vb) {                                                    \
            std::cerr << "[ASSERT_NE FAILED] " << #a << " != " << #b         \
                      << " at " << __FILE__ << ":" << __LINE__               \
                      << " (both " << _va << ")\n";                          \
            throw std::runtime_error("ASSERT_NE failed");                    \
        }                                                                    \
    } while (0)


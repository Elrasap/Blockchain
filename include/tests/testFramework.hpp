#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <array>
#include <iomanip>
#include <string>
#include <cstdint>
#include <type_traits>

/* ============================================================
   TYPE TRAITS
   ============================================================ */

template<typename T>
struct is_std_vector : std::false_type {};

template<typename U>
struct is_std_vector<std::vector<U>> : std::true_type {};

template<typename T>
struct is_std_array : std::false_type {};

template<typename U, std::size_t N>
struct is_std_array<std::array<U, N>> : std::true_type {};

/* ============================================================
   BYTE → HEX Helper
   ============================================================ */
template<typename T>
inline std::string byteToHex(T b) {
    std::ostringstream oss;
    oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return oss.str();
}

/* ============================================================
   toString() – MASTER DISPATCHER
   ============================================================ */

// Arrays (hashes, merkle roots, signatures ...)
template<typename T>
inline std::string toStringArray(const T& arr) {
    std::ostringstream oss;
    oss << "0x";
    for (auto b : arr) {
        oss << byteToHex(b);
    }
    return oss.str();
}

// Vectors
template<typename T>
inline std::string toStringVector(const std::vector<T>& vec) {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (auto& x : vec) {
        if (!first) oss << ", ";
        first = false;
        oss << toString(x);
    }
    oss << "]";
    return oss.str();
}

// Primitive numeric fallback
template<typename T>
inline std::string toStringNumber(const T& v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

// uint8_t override (avoid printing ASCII char)
inline std::string toString(const uint8_t& v) {
    return std::to_string(v);
}

// General dispatcher
template<typename T>
inline std::string toString(const T& v) {
    if constexpr (is_std_array<T>::value) {
        return toStringArray(v);

    } else if constexpr (is_std_vector<T>::value) {
        return toStringVector(v);

    } else if constexpr (std::is_arithmetic_v<T>) {
        return toStringNumber(v);

    } else {
        // Fallback: use operator<<
        std::ostringstream oss;
        oss << v;
        return oss.str();
    }
}

/* ============================================================
   ASSERT FUNCTIONS
   ============================================================ */

template<typename A, typename B>
inline void assertEq(const A& a, const B& b,
                     const char* exprA, const char* exprB,
                     const char* file, int line)
{
    if (!(a == b)) {
        std::cerr << "[ASSERT_EQ FAILED] " << exprA << " == " << exprB
                  << " at " << file << ":" << line << "\n"
                  << "  got:      " << toString(a) << "\n"
                  << "  expected: " << toString(b) << "\n";
        throw std::runtime_error("ASSERT_EQ failed");
    }
}

template<typename A, typename B>
inline void assertNe(const A& a, const B& b,
                     const char* exprA, const char* exprB,
                     const char* file, int line)
{
    if (a == b) {
        std::cerr << "[ASSERT_NE FAILED] " << exprA << " != " << exprB
                  << " at " << file << ":" << line << "\n"
                  << "  value: " << toString(a) << "\n";
        throw std::runtime_error("ASSERT_NE failed");
    }
}

#define ASSERT_EQ(a,b) assertEq((a),(b), #a,#b, __FILE__, __LINE__)
#define ASSERT_NE(a,b) assertNe((a),(b), #a,#b, __FILE__, __LINE__)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[ASSERT_TRUE FAILED] " << #cond \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            throw std::runtime_error("ASSERT_TRUE failed"); \
        } \
    } while(0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::cerr << "[ASSERT_FALSE FAILED] " << #cond \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            throw std::runtime_error("ASSERT_FALSE failed"); \
        } \
    } while(0)

/* ============================================================
   TEST REGISTRY
   ============================================================ */

#define TEST_CASE(name) \
    void name(); \
    struct name##_registrar { \
        name##_registrar() { registerTest(#name, name); } \
    } name##_instance; \
    void name()

inline std::vector<std::pair<std::string, void(*)()>>& getTests() {
    static std::vector<std::pair<std::string, void(*)()>> tests;
    return tests;
}

inline void registerTest(const std::string& name, void(*f)()) {
    getTests().push_back({name, f});
}


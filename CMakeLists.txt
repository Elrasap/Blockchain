cmake_minimum_required(VERSION 3.20)

project(Blockchain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ==========================================================
# Dependencies
# ==========================================================

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# ==========================================================
# Include paths
# ==========================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBSODIUM_INCLUDE_DIRS}
)

# ==========================================================
# Core Source Files
# ==========================================================

file(GLOB_RECURSE BLOCKCHAIN_CORE_SRC
    src/core/*.cpp
    src/storage/*.cpp
    src/analytics/*.cpp
    src/cluster/*.cpp
    src/obs/*.cpp
    src/ops/*.cpp
    src/metrics/*.cpp
    src/web/*.cpp
    src/dnd/*.cpp
    src/dashboard/*.cpp
    src/network/*.cpp
    src/recovery/*.cpp
    src/security/*.cpp
    src/upgrade/*.cpp
    src/incident/*.cpp
    src/light/*.cpp
    src/util/*.cpp
)

# ==========================================================
# Main Node Binary
# ==========================================================

add_executable(blockchain_node
    src/main.cpp
    ${BLOCKCHAIN_CORE_SRC}
)

target_link_libraries(blockchain_node
    PRIVATE
        SQLite::SQLite3
        OpenSSL::Crypto
        CURL::libcurl
        ${LIBSODIUM_LIBRARIES}
)

# ==========================================================
# Test Sources
# ==========================================================

file(GLOB TEST_SOURCES
    src/tests/testFramework.hpp
    src/tests/testRunner.cpp
    src/tests/unit_block_tests.cpp
    src/tests/unit_dnd_apply_tests.cpp
    src/tests/selftest_chain_dnd.cpp
    src/tests/fuzz_encounter_state.cpp
)

# ==========================================================
# Test Binary
# ==========================================================

add_executable(blockchain_tests
    ${TEST_SOURCES}
    ${BLOCKCHAIN_CORE_SRC}    # Tests brauchen alle Core-Module
)

target_link_libraries(blockchain_tests
    PRIVATE
        SQLite::SQLite3
        OpenSSL::Crypto
        CURL::libcurl
        ${LIBSODIUM_LIBRARIES}
)

# ==========================================================
# Output
# ==========================================================

message(STATUS "===============================================")
message(STATUS " Build configuration complete")
message(STATUS " Node binary:        blockchain_node")
message(STATUS " Test binary:        blockchain_tests")
message(STATUS " Sodium include dir: ${LIBSODIUM_INCLUDE_DIRS}")
message(STATUS " Sodium libs:        ${LIBSODIUM_LIBRARIES}")
message(STATUS "===============================================")


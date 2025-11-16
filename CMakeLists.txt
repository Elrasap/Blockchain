cmake_minimum_required(VERSION 3.20)

project(Blockchain LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ====================================================================================
# Dependencies
# ====================================================================================

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# libsodium (über pkg-config)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBSODIUM_INCLUDE_DIRS}
)

# ====================================================================================
# Source Files
# ====================================================================================

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
    src/tests/regressionReporter.cpp
    src/tests/regressionRunner.cpp
    src/util/dummyCrypto.cpp       # <--- hier dein Stub dazu
)

# ====================================================================================
# Main binary
# ====================================================================================

add_executable(blockchain_node
    src/main.cpp
    ${BLOCKCHAIN_CORE_SRC}
)

# ====================================================================================
# Link libraries
# ====================================================================================

target_link_libraries(blockchain_node
    PRIVATE
        SQLite::SQLite3
        OpenSSL::Crypto
        CURL::libcurl
        ${LIBSODIUM_LIBRARIES}
)

message(STATUS "Build configuration complete.")


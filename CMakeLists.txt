cmake_minimum_required(VERSION 3.16)
project(blockchain_node LANGUAGES CXX)

# =====================================================================
# C++20 CONFIG
# =====================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-missing-field-initializers
        -Wno-deprecated
    )
endif()

# =====================================================================
# DEPENDENCIES
# =====================================================================

# libsodium
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# CURL
find_package(CURL REQUIRED)

# OPENSSL
find_package(OpenSSL REQUIRED)

# SQLITE
find_package(SQLite3 REQUIRED)

# =====================================================================
# GATHER ALL SOURCE FILES
# =====================================================================

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

# Extra file: some distros do not glob this consistently
set(EXTRA_SOURCES
    src/incident/forensicsReporter.cpp
)

# =====================================================================
# EXECUTABLE
# =====================================================================

add_executable(blockchain_node
    ${SRC_FILES}
    ${EXTRA_SOURCES}
)

# =====================================================================
# INCLUDE DIRECTORIES
# =====================================================================
target_include_directories(blockchain_node PRIVATE
    include
    ${SODIUM_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# =====================================================================
# LINK LIBRARIES
# =====================================================================
target_link_libraries(blockchain_node
    ${SODIUM_LIBRARIES}
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
    pthread
)

# GCC < 9 requires manual filesystem lib
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        target_link_libraries(blockchain_node stdc++fs)
    endif()
endif()

# =====================================================================
# STATUS OUTPUT
# =====================================================================
message(STATUS "")
message(STATUS "===================================================")
message(STATUS "  Building blockchain_node with:")
message(STATUS "  - C++20")
message(STATUS "  - libsodium: ${SODIUM_LIBRARIES}")
message(STATUS "  - curl:      ${CURL_LIBRARIES}")
message(STATUS "  - openssl:   ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "  - sqlite3:   ${SQLite3_LIBRARIES}")
message(STATUS "  - sources:   ${SRC_FILES}")
message(STATUS "===================================================")
message(STATUS "")


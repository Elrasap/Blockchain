cmake_minimum_required(VERSION 3.16)
project(DndBlockchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# Sanitizer (korrekt!)
# -------------------------------------------------------
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fno-omit-frame-pointer)
endif()

# -------------------------------------------------------
# Dependencies
# -------------------------------------------------------
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

find_library(SODIUM_LIB sodium)
find_library(SQLITE3_LIB sqlite3)

include_directories(
    include
    include/core
    include/network
    include/web
    include/storage
    include/dnd
    include/metrics
    include/upgrade
    include/analytics
    include/ops
    include/light
    include/thirdparty
)

# -------------------------------------------------------
# Node sources
# -------------------------------------------------------
file(GLOB_RECURSE NODE_SRC
    src/core/*.cpp
    src/network/*.cpp
    src/web/*.cpp
    src/storage/*.cpp
    src/metrics/*.cpp
    src/dnd/*.cpp
    src/upgrade/*.cpp
    src/analytics/*.cpp
    src/ops/*.cpp
    src/light/*.cpp
    src/release/*.cpp
    src/obs/*.cpp
)

# Remove unused legacy dirs
list(FILTER NODE_SRC EXCLUDE REGEX "backup")
list(FILTER NODE_SRC EXCLUDE REGEX "agent")

# -------------------------------------------------------
# Node executable
# -------------------------------------------------------
add_executable(blockchain_node
    ${NODE_SRC}
    src/main.cpp
)

target_link_libraries(blockchain_node
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
    ${SQLITE3_LIB}
    ${SODIUM_LIB}
    CURL::libcurl
)

# -------------------------------------------------------
# No tests in this version
# -------------------------------------------------------
message(STATUS "Tests disabled – Option A active")


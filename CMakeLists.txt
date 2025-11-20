cmake_minimum_required(VERSION 3.16)
project(DndBlockchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-deprecated-declarations)
endif()

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
# NODE SOURCES
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
# NODE EXECUTABLE
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
# NO TESTS IN THIS VERSION
# -------------------------------------------------------

message(STATUS "Tests disabled – Option A active")


cmake_minimum_required(VERSION 3.16)
project(DndBlockchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-deprecated-declarations)
endif()

# Threading
find_package(Threads REQUIRED)

# OpenSSL
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(
    include
    include/thirdparty
    include/core
    include/network
    include/web
    include/storage
    include/dnd
    include/metrics
    include/tests
)

# ============================================================
# NODE SOURCES (exclude backup + agent)
# ============================================================

file(GLOB_RECURSE NODE_SRC
    src/core/*.cpp
    src/network/*.cpp
    src/web/*.cpp
    src/storage/*.cpp
    src/metrics/*.cpp
    src/dnd/*.cpp
)

# Ensure backups excluded
list(FILTER NODE_SRC EXCLUDE REGEX "backup")
list(FILTER NODE_SRC EXCLUDE REGEX "agent")

# Add main.cpp manually
set(NODE_MAIN src/main.cpp)

add_executable(blockchain_node
    ${NODE_SRC}
    ${NODE_MAIN}
)

target_link_libraries(blockchain_node
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
)

# ============================================================
# TEST SOURCES (clean separation)
# ============================================================

file(GLOB_RECURSE TEST_SRC
    src/tests/*.cpp
)

add_executable(blockchain_tests
    ${TEST_SRC}
)

target_link_libraries(blockchain_tests
    Threads::Threads
    OpenSSL::Crypto
)


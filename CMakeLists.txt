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

# Include Dir
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

# Source files
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

# MAIN EXECUTABLE
add_executable(blockchain_node
    ${SRC_FILES}
    src/main.cpp        # ensure main is included
)

target_link_libraries(blockchain_node
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
)

# TEST EXECUTABLE
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


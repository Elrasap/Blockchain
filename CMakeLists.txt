cmake_minimum_required(VERSION 3.16)
project(DndBlockchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    include/tests
    include/thirdparty
)

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
list(FILTER NODE_SRC EXCLUDE REGEX "backup")
list(FILTER NODE_SRC EXCLUDE REGEX "agent")

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
    CURL::libcurl     # <---- Einheitlich
)

file(GLOB_RECURSE TEST_SRC
    src/tests/*.cpp
)

add_executable(blockchain_tests
    ${TEST_SRC}
    ${NODE_SRC}
)

target_link_libraries(blockchain_tests
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
    ${SQLITE3_LIB}
    ${SODIUM_LIB}
    CURL::libcurl     # <---- Einheitlich
)


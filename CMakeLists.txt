cmake_minimum_required(VERSION 3.16)
project(DndBlockchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Build with runtime sanitizers" OFF)
set(SANITIZERS "address,undefined" CACHE STRING
    "Comma-separated sanitizer list used when ENABLE_SANITIZERS is ON")

if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=${SANITIZERS} -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${SANITIZERS} -fno-omit-frame-pointer)
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_library(SODIUM_LIB sodium REQUIRED)
find_library(SQLITE3_LIB sqlite3 REQUIRED)

set(PROJECT_INCLUDE_DIRS
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

file(GLOB_RECURSE NODE_SRC CONFIGURE_DEPENDS
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
list(FILTER NODE_SRC EXCLUDE REGEX "/backup/")
list(FILTER NODE_SRC EXCLUDE REGEX "/agent/")

add_library(dnd_blockchain STATIC ${NODE_SRC})
target_include_directories(dnd_blockchain PUBLIC ${PROJECT_INCLUDE_DIRS})
target_link_libraries(dnd_blockchain PUBLIC
    Threads::Threads
    OpenSSL::Crypto
    OpenSSL::SSL
    ${SQLITE3_LIB}
    ${SODIUM_LIB}
    CURL::libcurl
)

add_executable(blockchain_node src/main.cpp)
target_link_libraries(blockchain_node PRIVATE dnd_blockchain)

include(CTest)
if(BUILD_TESTING)
    add_executable(blockchain_tests
        src/tests/testRunner.cpp
        src/tests/hardening_tests.cpp
        src/tests/fuzz_encounter_state.cpp
    )
    target_link_libraries(blockchain_tests PRIVATE dnd_blockchain)
    add_test(NAME blockchain_tests COMMAND blockchain_tests)
    set_tests_properties(blockchain_tests PROPERTIES TIMEOUT 60)
endif()

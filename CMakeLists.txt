cmake_minimum_required(VERSION 3.16)
project(BlockchainNode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

# ---- Quellen automatisch finden ----
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(blockchain_node ${SOURCES})

# ---- OpenSSL (für SHA-256) ----
find_package(OpenSSL REQUIRED)
target_link_libraries(blockchain_node PRIVATE OpenSSL::Crypto)

# ---- Libsodium (für Ed25519 Signaturen) ----
# ACHTUNG: Kein find_package, sondern direktes Linken
target_link_libraries(blockchain_node PRIVATE sodium)

# ---- Include Paths ----
target_include_directories(blockchain_node PRIVATE include)


cmake_minimum_required(VERSION 3.16)
project(BlockchainNode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    include
    include/thirdparty
    include/dnd
    include/core
    include/ops
    include/analytics
    include/storage
    include/web
)

# Source files
set(SOURCES

    # MAIN
    src/main.cpp

    # CORE
    src/core/transaction.cpp
    src/core/mempool.cpp
    src/core/crypto.cpp

    # OPS
    src/ops/reliabilityGuard.cpp

    # ANALYTICS
    src/analytics/rtoRpoAnalyzer.cpp
    src/analytics/trendAnalyzer.cpp
    src/analytics/trendReporter.cpp

    # STORAGE
    src/storage/historyStore.cpp

    # WEB
    src/web/dashboardServer.cpp
    src/web/dashboard_assets.cpp

    # RELEASE
    src/release/checksummer.cpp
    src/release/releaseManifest.cpp

    # DND SYSTEM
    src/dnd/character.cpp
    src/dnd/payload.cpp
    src/dnd/patch.cpp
    src/dnd/dndCharacterService.cpp

    src/dnd/monster.cpp
    src/dnd/encounter.cpp
)

# Build executable
add_executable(blockchain_node ${SOURCES})

# External libs
target_link_libraries(blockchain_node
    pthread
)

# Optional: print summary
message(STATUS "BlockchainNode build configured.")


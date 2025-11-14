cmake_minimum_required(VERSION 3.16)
project(blockchain_node LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CI_BUILD "Disable components that define extra mains" OFF)

# ---------------------------------------
# External Libraries
# ---------------------------------------
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# libsodium (Ed25519)
find_library(SODIUM_LIBRARY sodium REQUIRED)

# ---------------------------------------
# Includes
# ---------------------------------------
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/httplib
)

# ---------------------------------------
# Collect sources
# ---------------------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Remove files that create multiple main() functions
list(FILTER SRC_FILES EXCLUDE REGEX "chaosCiRunner.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "testMain.cpp")

# OBS contains duplicate MetricsServer -> remove one
list(FILTER SRC_FILES EXCLUDE REGEX "src/obs/metricsServer.cpp")

# Some modules require the existence of ForensicsReporter.cpp (missing)
# For now disable IncidentHandler if the file does not exist
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/incident/forensicsReporter.cpp")
    list(FILTER SRC_FILES EXCLUDE REGEX "incidentHandler.cpp")
endif()

# ---------------------------------------
# Create executable
# ---------------------------------------
add_executable(blockchain_node ${SRC_FILES})

# ---------------------------------------
# Link libs
# ---------------------------------------
target_link_libraries(blockchain_node
    PRIVATE
        SQLite::SQLite3
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        ${SODIUM_LIBRARY}
        pthread
        dl
)

# Output dir
set_target_properties(blockchain_node PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)


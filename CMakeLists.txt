cmake_minimum_required(VERSION 3.10)
project(SpeedCloudflareCli)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

include(FetchContent)
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG master
)
FetchContent_MakeAvailable(yyjson)

# Ensure YYJSON_WRITE_MAX_DEPTH is set for the yyjson target
if(TARGET yyjson)
    target_compile_definitions(yyjson INTERFACE YYJSON_WRITE_MAX_DEPTH=32)
endif()

add_definitions(-DYYJSON_WRITE_MAX_DEPTH=32)

add_executable(SpeedCloudflareCli
    main.cpp
    chalk.cpp
    stats.cpp
    benchmarks.cpp
    cli_args.cpp
    output.cpp
    json_helpers.cpp
    diagnostics.cpp
    sysinfo.cpp
    network.cpp
)

target_link_libraries(SpeedCloudflareCli PRIVATE pthread ${CURL_LIBRARIES} yyjson)

# nlohmann_json and json-schema-validator integration
include(cmake/nlohmann_json_schema.cmake)

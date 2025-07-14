cmake_minimum_required(VERSION 3.25)
project(SpeedCloudflareCli)

# Enforce C++17 standard globally (no GNU extensions by default)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Centralized user source directory for all user code
set(USER_SRC_DIR "${PROJECT_SOURCE_DIR}/src")

# Note: All static analysis and formatting is controlled by .clang-tidy and .clang-format in the project root.
# Do not add per-target or per-directory clang-tidy/format config elsewhere.

# Only set GNU extension checks for cross-compiling (e.g., ARM build)
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

    # Check if the cross-compiler supports GNU extensions (gnu++17 or gnu++1z)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=gnu++17" COMPILER_SUPPORTS_GNUXX17)
    check_cxx_compiler_flag("-std=gnu++1z" COMPILER_SUPPORTS_GNUXX1Z)

    if(COMPILER_SUPPORTS_GNUXX17)
        message(STATUS "Compiler supports -std=gnu++17 (GNU extensions enabled)")
    elseif(COMPILER_SUPPORTS_GNUXX1Z)
        message(STATUS "Compiler supports -std=gnu++1z (GNU extensions enabled)")
    else()
        message(WARNING "Compiler does NOT support -std=gnu++17 or -std=gnu++1z. Falling back to -std=c++17.")
    endif()

    # Custom target to run clang-tidy manually on host, using ARM build's compile_commands.json
    file(GLOB SRC_FILES "${USER_SRC_DIR}/*.cpp")
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        add_custom_target(
            clang-tidy-arm
            COMMAND ${CLANG_TIDY_EXE}
                -p ${CMAKE_BINARY_DIR}
                --extra-arg-before=-std=c++17
                ${SRC_FILES}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Running clang-tidy on user sources using ARM build's compile_commands.json and .clang-tidy"
            VERBATIM
        )
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependency: CURL (system)
find_package(CURL REQUIRED)

# Dependency: yyjson (via FetchContent, see cmake/ibireme_yyjson.cmake)
include(cmake/ibireme_yyjson.cmake)

# Gather all user source files
file(GLOB PROJECT_SOURCES "${USER_SRC_DIR}/*.cpp")

# Main executable target for user code only
add_executable(SpeedCloudflareCli)
target_sources(SpeedCloudflareCli PRIVATE ${PROJECT_SOURCES})

# nlohmann_json and json-schema-validator integration (see cmake/nlohmann_json_schema.cmake)
include(cmake/nlohmann_json_schema.cmake)

# Target-specific include directories (user code and dependencies only)
target_include_directories(SpeedCloudflareCli PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/_deps/yyjson-src/src
    ${json-schema-validator_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/_deps/json-schema-validator-src/src
    ${USER_SRC_DIR}
)

# Target-specific compile definitions and link libraries
# Set YYJSON_WRITE_MAX_DEPTH for both SpeedCloudflareCli and yyjson (if available)
target_compile_definitions(SpeedCloudflareCli PRIVATE YYJSON_WRITE_MAX_DEPTH=32)
if(TARGET yyjson)
    target_compile_definitions(yyjson INTERFACE YYJSON_WRITE_MAX_DEPTH=32)
endif()

target_link_libraries(SpeedCloudflareCli PRIVATE pthread ${CURL_LIBRARIES} yyjson)

# Enable clang-tidy for user target only (native builds)
if(NOT CMAKE_CROSSCOMPILING)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set_target_properties(SpeedCloudflareCli PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endif()

# End of CMakeLists.txt

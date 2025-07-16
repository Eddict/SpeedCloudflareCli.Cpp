cmake_minimum_required(VERSION 3.25)
project(SpeedCloudflareCli)

# Enforce C++17 standard globally (no GNU extensions by default)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Centralized user source directory for all user code
set(USER_SRC_DIR "${PROJECT_SOURCE_DIR}/src")

# Option to enable gprof profiling instrumentation
option(ENABLE_GPROF "Enable gprof profiling instrumentation" OFF)
if(ENABLE_GPROF)
    message(STATUS "gprof instrumentation enabled: adding -pg to compile and link flags")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()

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

# === Embed JSON schemas as C++ raw string headers ===
macro(embed_json_raw INPUT OUTPUT VAR)
  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -DINPUT=${INPUT}
      -DOUTPUT=${OUTPUT}
      -DVAR=${VAR}
      -P ${CMAKE_SOURCE_DIR}/cmake/embed_json_as_raw_string.cmake
  )
endmacro()

embed_json_raw(
  "${CMAKE_SOURCE_DIR}/schemas/result.schema.json"
  "${CMAKE_BINARY_DIR}/embedded_result_schema.h"
  embedded_result_schema
)
embed_json_raw(
  "${CMAKE_SOURCE_DIR}/schemas/summary.schema.json"
  "${CMAKE_BINARY_DIR}/embedded_summary_schema.h"
  embedded_summary_schema
)

include_directories("${CMAKE_BINARY_DIR}")

# IWYU integration: add custom target using mapping file (fixed: use SRC_FILES for sources)
file(GLOB SRC_FILES "${USER_SRC_DIR}/*.cpp")
find_program(IWYU_EXE NAMES include-what-you-use)
if(IWYU_EXE)
    add_custom_target(
        iwyu
        COMMAND python3 /usr/local/bin/iwyu_tool.py
            -p ${CMAKE_BINARY_DIR}
            ${SRC_FILES}
            -- -I/usr/lib/gcc/x86_64-linux-gnu/12/include
            # -- --mapping_file=${CMAKE_SOURCE_DIR}/iwyu_mappings.imp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running include-what-you-use with mapping file for all user sources"
        VERBATIM
    )
endif()

# End of CMakeLists.txt

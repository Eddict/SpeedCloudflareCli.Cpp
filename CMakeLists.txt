cmake_minimum_required(VERSION 3.25)
project(SpeedCloudflareCli)

# Enforce C++17 standard globally (no GNU extensions by default)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Centralized user source directory for all user code
set(USER_SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Gather all user source files
file(GLOB PROJECT_SOURCES "${USER_SRC_DIR}/*.cpp")

# Main executable target for user code only
add_executable(SpeedCloudflareCli)
target_sources(SpeedCloudflareCli PRIVATE ${PROJECT_SOURCES})

# Target-specific include directories (user code and dependencies only)
target_include_directories(SpeedCloudflareCli PRIVATE
    ${USER_SRC_DIR}
)

target_link_libraries(SpeedCloudflareCli PRIVATE pthread)

include_directories("${CMAKE_BINARY_DIR}")

# Only set GNU extension checks for cross-compiling (e.g., ARM build)
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

    # add/set this macro to enable conditional code (#ifdef) 
    add_compile_definitions(CROSSCOMPILING_BUILD)

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
endif()


# Dependency: yyjson (via FetchContent, see cmake/ibireme_yyjson.cmake)
include(cmake/ibireme_yyjson.cmake)
# nlohmann_json and json-schema-validator integration (see cmake/nlohmann_json_schema.cmake)
include(cmake/nlohmann_json_schema.cmake)
# Dependency: Boost (see cmake/boost_integration.cmake)
include(cmake/boost_integration.cmake)
# Dependency: cpp-httplib (via FetchContent, see cmake/yhirose_cpp-httplib.cmake)
# include(cmake/yhirose_cpp-httplib.cmake)
# Optional: clang-tidy integration
include(cmake/clang-tidy.cmake)
# Optional: IWYU integration
include(cmake/iwyu.cmake)
# Optional: GPROF compiling instrumentation
include(cmake/gprof.cmake)
#include(cmake/update_libc_linker_script.cmake)

# End of CMakeLists.txt

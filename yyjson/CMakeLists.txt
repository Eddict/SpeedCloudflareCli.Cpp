# yyjson CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(yyjson C)

# Download yyjson source files automatically if not present
include(FetchContent)
FetchContent_Declare(
    yyjson_official
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG master
)
FetchContent_MakeAvailable(yyjson_official)

add_library(yyjson STATIC IMPORTED)
set_target_properties(yyjson PROPERTIES
    IMPORTED_LOCATION ${yyjson_official_SOURCE_DIR}/src/yyjson.c
    INTERFACE_INCLUDE_DIRECTORIES ${yyjson_official_SOURCE_DIR}/src
)

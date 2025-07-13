cmake_minimum_required(VERSION 3.25)
project(SpeedCloudflareCli)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

include(FetchContent)
FetchContent_Declare(
    yyjson
#    [SYSTEM]
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG master
)
FetchContent_MakeAvailable(yyjson)
#EF: https://discourse.cmake.org/t/how-to-prevent-clang-tidy-to-check-sources-added-with-fetchcontents/2871/3
#foreach (lang IN ITEMS C CXX)
#  set("CMAKE_${lang}_CLANG_TIDY_save" "${CMAKE_${lang}_CLANG_TIDY}")
#  set("CMAKE_${lang}_CLANG_TIDY" "")
#endforeach ()
#FetchContent()
#FetchContent_MakeAvailable(yyjson)
#foreach (lang IN ITEMS C CXX)
#  set("CMAKE_${lang}_CLANG_TIDY" "${CMAKE_${lang}_CLANG_TIDY_save}")
#endforeach ()

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

#target_include_directories(yyjson PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include") # or whatever path
#target_include_directories(yyjson PUBLIC  
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/yyjson>  
#    $<INSTALL_INTERFACE:include/yyjson>  # <prefix>/include/yyjson
#)

#set_property(TARGET yyjson PROPERTY SYSTEM TRUE)

# nlohmann_json and json-schema-validator integration
include(cmake/nlohmann_json_schema.cmake)

# yyjson integration for SpeedCloudflareCli

# Only set BUILD_SHARED_LIBS if not already set by parent
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
  yyjson
  SYSTEM
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  GIT_TAG master
)
FetchContent_MakeAvailable(yyjson)

# Target-specific compile definitions and link libraries
# Set YYJSON_WRITE_MAX_DEPTH for both SpeedCloudflareCli and yyjson (if available)
target_compile_definitions(SpeedCloudflareCli PRIVATE YYJSON_WRITE_MAX_DEPTH=32)
if(TARGET yyjson)
    target_compile_definitions(yyjson INTERFACE YYJSON_WRITE_MAX_DEPTH=32)
endif()

target_link_libraries(SpeedCloudflareCli PRIVATE yyjson)

target_include_directories(SpeedCloudflareCli PRIVATE
    ${CMAKE_BINARY_DIR}/_deps/yyjson-src/src
)

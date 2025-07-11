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

add_executable(SpeedCloudflareCli main.cpp chalk.cpp stats.cpp)

target_link_libraries(SpeedCloudflareCli PRIVATE pthread ${CURL_LIBRARIES} yyjson)

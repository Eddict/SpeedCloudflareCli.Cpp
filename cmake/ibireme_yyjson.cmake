# yyjson integration for SpeedCloudflareCli
#
# This snippet is to be included in your main CMakeLists.txt to add yyjson via FetchContent.

include(FetchContent)

FetchContent_Declare(
  yyjson
  SYSTEM
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  GIT_TAG master
)
FetchContent_MakeAvailable(yyjson)

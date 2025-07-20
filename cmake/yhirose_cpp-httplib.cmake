# Add cpp-httplib single header
include(FetchContent)
FetchContent_Declare(
  cpp-httplib
  SYSTEM
  URL https://github.com/yhirose/cpp-httplib/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(cpp-httplib)

# To suppress the warning about "cpp-httplib doesn't support 32-bit platforms. Please use a 64-bit compiler."
# Use the command sed to delete the #warning line after fetching the header.
if(CMAKE_CROSSCOMPILING)
  execute_process(
      COMMAND sed -i "/^#warning[[:space:]]*\\\\$/ {N; d; }" ${cpp-httplib_SOURCE_DIR}/httplib.h
  )
endif()

# Create an INTERFACE target for the header-only library
add_library(cpp-httplib INTERFACE)
target_include_directories(cpp-httplib SYSTEM INTERFACE ${cpp-httplib_SOURCE_DIR})
target_compile_definitions(cpp-httplib INTERFACE CPPHTTPLIB_NO_ZSTD)
target_compile_definitions(cpp-httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)

# OpenSSL for HTTPS support in cpp-httplib
find_package(OpenSSL REQUIRED)
target_link_libraries(SpeedCloudflareCli PRIVATE cpp-httplib OpenSSL::SSL OpenSSL::Crypto)

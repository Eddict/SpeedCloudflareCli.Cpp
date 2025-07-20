# Boost HTTP/Networking integration for SpeedCloudflareCli
include(FetchContent)
FetchContent_Declare(
  boost_integration
  SYSTEM
  OVERRIDE_FIND_PACKAGE 
)

# Find Boost (headers and libraries)
find_package(Boost REQUIRED COMPONENTS system filesystem thread)

# Mark Boost as SYSTEM to suppress warnings
if(Boost_FOUND)
    add_library(boost_integration INTERFACE)
    target_include_directories(boost_integration SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
    target_link_libraries(boost_integration INTERFACE ${Boost_LIBRARIES})
endif()

# Link SpeedCloudflareCli against Boost and OpenSSL (direct library paths for cross-compiling)
target_link_libraries(SpeedCloudflareCli PRIVATE boost_integration
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
)

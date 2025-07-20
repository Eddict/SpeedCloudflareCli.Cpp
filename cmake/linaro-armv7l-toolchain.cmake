# CMake toolchain file for cross-compiling to ARMv7 (arm-linux-gnueabihf)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   /root/gcc-linaro_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /root/gcc-linaro_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)

# Add Entware paths for libraries and includes
set(ENTWARE_LIB_PATH /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/lib)
set(ENTWARE_INCLUDE_PATH /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/include)

# Update sysroot and find root path to include Entware
set(CMAKE_SYSROOT /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/libc)
set(CMAKE_FIND_ROOT_PATH
    /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/libc
    ${ENTWARE_LIB_PATH}
    ${ENTWARE_INCLUDE_PATH}
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Add Entware lib to linker flags
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/lib -L${ENTWARE_LIB_PATH}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -L${ENTWARE_LIB_PATH} -L/root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/lib")

# Boost and OpenSSL cross-compilation hints
set(BOOST_ROOT /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware)
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_LIBRARY_DIRS
    /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/lib
    /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/opt/lib)
set(Boost_INCLUDE_DIRS
    /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/include
    /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/opt/include)
set(OPENSSL_ROOT_DIR /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware)
set(OPENSSL_CRYPTO_LIBRARY /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/opt/lib/libcrypto.so)
set(OPENSSL_SSL_LIBRARY /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/opt/lib/libssl.so)
set(OPENSSL_INCLUDE_DIR /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/entware/opt/include)

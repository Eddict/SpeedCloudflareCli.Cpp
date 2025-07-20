# CMake toolchain file for Entware ARMv7 sysroot
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   /root/gcc-linaro_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /root/gcc-linaro_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)

set(ENTWARE_SYSROOT /root/entware-armv7sf-k3.2-sysroot)
set(CMAKE_SYSROOT ${ENTWARE_SYSROOT})

set(CMAKE_FIND_ROOT_PATH
    ${ENTWARE_SYSROOT}
    ${ENTWARE_SYSROOT}/opt/lib
    ${ENTWARE_SYSROOT}/opt/lib/gcc/arm-openwrt-linux-gnueabi/8.4.0
    # ${ENTWARE_SYSROOT}/lib
    # ${ENTWARE_SYSROOT}/usr/lib
    ${ENTWARE_SYSROOT}/opt/include
    ${ENTWARE_SYSROOT}/opt/lib/gcc/arm-openwrt-linux-gnueabi/8.4.0/include
    # ${ENTWARE_SYSROOT}/include
    # ${ENTWARE_SYSROOT}/usr/include
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${ENTWARE_SYSROOT}/opt/lib -L${ENTWARE_SYSROOT}/opt/lib/gcc/arm-openwrt-linux-gnueabi/8.4.0 -L${ENTWARE_SYSROOT}/lib -L${ENTWARE_SYSROOT}/usr/lib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${ENTWARE_SYSROOT}/opt/lib -L${ENTWARE_SYSROOT}/opt/lib/gcc/arm-openwrt-linux-gnueabi/8.4.0")

# Boost and OpenSSL hints (update paths if needed)
set(BOOST_ROOT ${ENTWARE_SYSROOT})
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_LIBRARY_DIRS
    ${ENTWARE_SYSROOT}/opt/lib
    # ${ENTWARE_SYSROOT}/lib
    # ${ENTWARE_SYSROOT}/usr/lib
)
set(Boost_INCLUDE_DIRS
    ${ENTWARE_SYSROOT}/opt/include
    # ${ENTWARE_SYSROOT}/include
    # ${ENTWARE_SYSROOT}/usr/include
)
set(OPENSSL_ROOT_DIR ${ENTWARE_SYSROOT})
set(OPENSSL_CRYPTO_LIBRARY ${ENTWARE_SYSROOT}/opt/lib/libcrypto.so)
set(OPENSSL_SSL_LIBRARY ${ENTWARE_SYSROOT}/opt/lib/libssl.so)
set(OPENSSL_INCLUDE_DIR ${ENTWARE_SYSROOT}/opt/include)

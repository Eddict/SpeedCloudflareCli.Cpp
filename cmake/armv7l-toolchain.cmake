# CMake toolchain file for cross-compiling to ARMv7 (arm-linux-gnueabihf)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   /root/gcc-linaro_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /root/gcc-linaro_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)

set(CMAKE_SYSROOT /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/libc)
set(CMAKE_FIND_ROOT_PATH /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/libc)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

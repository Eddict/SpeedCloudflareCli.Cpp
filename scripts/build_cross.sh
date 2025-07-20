#!/bin/bash
set -e

TOOLCHAIN_ROOT="/root/gcc-linaro_arm-linux-gnueabihf"
SYSROOT="$TOOLCHAIN_ROOT/arm-linux-gnueabihf/libc"
ENTWARE_SYSROOT="$TOOLCHAIN_ROOT/arm-linux-gnueabihf/entware"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo "Project root folder: $PROJECT_ROOT"
echo "Toolchain root: $TOOLCHAIN_ROOT"
echo "Sysroot: $SYSROOT"
echo "Entware sysroot: $ENTWARE_SYSROOT"

# Clean build directory to ensure toolchain file is applied
rm -rf "$PROJECT_ROOT/build-arm"

# Step 1: Set linker script for shared libraries
cat <<EOF > "$ENTWARE_SYSROOT/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 )
EOF

cat <<EOF > "$SYSROOT/usr/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 )
EOF

echo "[INFO] Building shared libraries with toolchain file: cmake/linaro-armv7l-toolchain.cmake"
cmake -B "$PROJECT_ROOT/build-arm" -S "$PROJECT_ROOT" -DCMAKE_TOOLCHAIN_FILE=cmake/linaro-armv7l-toolchain.cmake -DBUILD_SHARED_LIBS=ON || true
make -C "$PROJECT_ROOT/build-arm" || true

# Step 2: Set linker script for executables/static builds
cat <<EOF > "$ENTWARE_SYSROOT/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 ../../lib/libc.a )
EOF

cat <<EOF > "$SYSROOT/usr/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 ../../lib/libc.a )
EOF

echo "[INFO] Building executables/static libraries with toolchain file: cmake/linaro-armv7l-toolchain.cmake"

cmake -B "$PROJECT_ROOT/build-arm" -S "$PROJECT_ROOT" -DCMAKE_TOOLCHAIN_FILE=cmake/linaro-armv7l-toolchain.cmake -DBUILD_SHARED_LIBS=OFF -DENABLE_GPROF=ON
make -C "$PROJECT_ROOT/build-arm" SpeedCloudflareCli && clang-tidy-arm

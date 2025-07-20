#!/bin/bash
set -e

ENTWARE_SYSROOT="/root/entware-armv7sf-k3.2-sysroot"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo "Project root folder: $PROJECT_ROOT"
echo "Entware sysroot: $ENTWARE_SYSROOT"

rm -rf "$PROJECT_ROOT/build-arm"

mkdir -p "$ENTWARE_SYSROOT/opt/lib" "$ENTWARE_SYSROOT/opt/usr/lib"
# Set linker script for shared libraries
cat <<EOF > "$ENTWARE_SYSROOT/opt/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 )
EOF

cat <<EOF > "$ENTWARE_SYSROOT/opt/usr/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 )
EOF

echo "[INFO] Building shared libraries with toolchain file: cmake/entware-armv7l-toolchain.cmake"
cmake -B "$PROJECT_ROOT/build-arm" -S "$PROJECT_ROOT" -DCMAKE_TOOLCHAIN_FILE=cmake/entware-armv7l-toolchain.cmake -DBUILD_SHARED_LIBS=ON || true
make -C "$PROJECT_ROOT/build-arm" || true

# Set linker script for executables/static builds
cat <<EOF > "$ENTWARE_SYSROOT/opt/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 ../../lib/libc.a )
EOF

cat <<EOF > "$ENTWARE_SYSROOT/opt/usr/lib/libc.so"
/* GNU ld script */
OUTPUT_FORMAT(elf32-littlearm)
GROUP ( ../../lib/libc.so.6 ../../lib/libc.a )
EOF

echo "[INFO] Building executables/static libraries with toolchain file: cmake/entware-armv7l-toolchain.cmake"
cmake -B "$PROJECT_ROOT/build-arm" -S "$PROJECT_ROOT" -DCMAKE_TOOLCHAIN_FILE=cmake/entware-armv7l-toolchain.cmake -DBUILD_SHARED_LIBS=OFF -DENABLE_GPROF=ON
make -C "$PROJECT_ROOT/build-arm" SpeedCloudflareCli && clang-tidy-arm

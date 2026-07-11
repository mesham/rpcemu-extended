# CMake toolchain file for cross-compiling RPCEmu to Windows (x86-64) with
# MinGW-w64. On an MSYS2 MINGW64 shell a native build needs no toolchain file;
# this is for cross-builds (e.g. from Linux) and for CI smoke checks.
#
#   cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${TOOLCHAIN_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Search host paths for programs, target paths for libraries/headers.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# pkg-config (used for SDL2 / libvncserver) must look at the target sysroot's
# .pc files, not the host's. Pin PKG_CONFIG_LIBDIR to the sysroot so a native
# pkg-config still resolves the cross libraries.
set(ENV{PKG_CONFIG_LIBDIR} "/usr/${TOOLCHAIN_PREFIX}/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
find_program(PKG_CONFIG_EXECUTABLE
    NAMES ${TOOLCHAIN_PREFIX}-pkg-config pkg-config)

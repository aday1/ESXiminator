# Cross-compile Windows x64 binaries using Zig's bundled clang + mingw-w64 sysroot
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# wrapper scripts created by the build environment (zig cc/c++ -target x86_64-windows-gnu)
set(CMAKE_C_COMPILER   "$ENV{ZIG_WRAP_DIR}/zig-cc")
set(CMAKE_CXX_COMPILER "$ENV{ZIG_WRAP_DIR}/zig-c++")
set(CMAKE_RC_COMPILER  "$ENV{ZIG_WRAP_DIR}/zig-rc")
set(CMAKE_RC_COMPILER_INIT "$ENV{ZIG_WRAP_DIR}/zig-rc")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(MINGW TRUE)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

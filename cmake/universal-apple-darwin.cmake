# the name of the target operating system
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR "x86_64")
set(TOOLCHAIN "universal-apple-darwin")

set(CMAKE_OSX_ARCHITECTURES "x86_64;i386")

# which compilers to use for C and C++
set(CMAKE_C_COMPILER "/usr/${TOOLCHAIN}/bin/x86_64-apple-darwin15-clang")
set(CMAKE_CXX_COMPILER "/usr/${TOOLCHAIN}/bin/x86_64-apple-darwin15-clang++")

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH "/usr/${TOOLCHAIN}/SDK/MacOSX10.11.sdk")

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "armv7h")
set(TOOLCHAIN "arm-linux-gnueabihf")

# which compilers to use for C and C++
set(CMAKE_C_COMPILER "${TOOLCHAIN}-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN}-g++")

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH "usr/${TOOLCHAIN}")

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(QEMU qemu-arm)

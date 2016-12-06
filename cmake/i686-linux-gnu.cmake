# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "i686")
set(TOOLCHAIN "i686-linux-gnu")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "c++ flags")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32" CACHE STRING "c flags")

set(STATIC_SERD "/opt/${TOOLCHAIN}/lib/libserd-0.a")
set(STATIC_SORD "/opt/${TOOLCHAIN}/lib/libsord-0.a")
set(STATIC_CAIRO "/opt/${TOOLCHAIN}/lib/libcairo.a")
set(STATIC_PIXMAN "/opt/${TOOLCHAIN}/lib/libpixman-1.a")

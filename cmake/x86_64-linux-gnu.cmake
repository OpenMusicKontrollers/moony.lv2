# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "x86_64")
set(TOOLCHAIN "x86_64-linux-gnu")

set(STATIC_LWS "/opt/${TOOLCHAIN}/lib/libwebsockets.a")
set(STATIC_SERD "/opt/${TOOLCHAIN}/lib/libserd-0.a")
set(STATIC_SORD "/opt/${TOOLCHAIN}/lib/libsord-0.a")
set(STATIC_CAIRO "/opt/${TOOLCHAIN}/lib/libcairo.a")
set(STATIC_PIXMAN "/opt/${TOOLCHAIN}/lib/libpixman-1.a")

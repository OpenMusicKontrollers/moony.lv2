cmake_minimum_required(VERSION 2.8)

project(timely.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

set(CMAKE_C_FLAGS "-std=gnu99 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,nodelete -Wl,-z,defs ${CMAKE_MODULE_LINKER_FLAGS}")
add_definitions("-D_GNU_SOURCE=1") # asprintf

set(TIMELY_MAJOR_VERSION 0)
set(TIMELY_MINOR_VERSION 1)
set(TIMELY_MICRO_VERSION 1)
set(TIMELY_VERSION "${TIMELY_MAJOR_VERSION}.${TIMELY_MINOR_VERSION}.${TIMELY_MICRO_VERSION}")
add_definitions("-DTIMELY_VERSION=\"${TIMELY_VERSION}\"")

set(DEST lib/lv2/timely.lv2)

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

# timely
add_library(timely MODULE
	test/timely.c)
target_link_libraries(timely m)
set_target_properties(timely PROPERTIES PREFIX "")
install(TARGETS timely DESTINATION ${DEST})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/timely.ttl DESTINATION ${DEST})

# manifest
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})

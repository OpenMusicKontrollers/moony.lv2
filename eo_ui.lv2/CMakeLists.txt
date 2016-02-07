cmake_minimum_required(VERSION 2.8)

project(eo.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

set(CMAKE_C_FLAGS "-std=gnu99 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,nodelete ${CMAKE_MODULE_LINKER_FLAGS}")
add_definitions("-D_GNU_SOURCE=1") # asprintf

set(EO_MAJOR_VERSION 0)
set(EO_MINOR_VERSION 1)
set(EO_MICRO_VERSION 1)

set(DEST lib/lv2/eo.lv2)

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

# eo
add_library(eo MODULE
	test/eo.c)
target_link_libraries(eo ${LIBS})
set_target_properties(eo PROPERTIES PREFIX "")
install(TARGETS eo DESTINATION ${DEST})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/eo.ttl DESTINATION ${DEST})

# manifest
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})

pkg_search_module(ELM REQUIRED elementary>=1.8)
include_directories(${ELM_INCLUDE_DIRS})
set(LIBS_UI ${LIBS_UI} ${ELM_LDFLAGS})

pkg_search_module(ECORE_X OPTIONAL ecore-x)

if((${ELM_VERSION} VERSION_EQUAL "1.9.0") OR (${ELM_VERSION} VERSION_GREATER "1.9.0"))
	add_definitions("-DELM_1_9")
endif()

if((DEFINED ECORE_X_FOUND) AND ((${ELM_VERSION} VERSION_EQUAL "1.13.0") OR (${ELM_VERSION} VERSION_GREATER "1.13.0")))
	message(STATUS "X11 UI wrap enabled")
	set(X11_UI_WRAP "")
	add_definitions("-DX11_UI_WRAP")
else()
	message(STATUS "X11 UI wrap disabled")
	set(X11_UI_WRAP "#")
endif()

# eo_ui
add_library(eo_ui MODULE
	test/eo_ui.c)
target_link_libraries(eo_ui ${LIBS_UI})
set_target_properties(eo_ui PROPERTIES PREFIX "")
install(TARGETS eo_ui DESTINATION ${DEST})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/lv2_white.png DESTINATION ${DEST})

cmake_minimum_required(VERSION 2.8)

project(moony.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tlsf-3.0)
include_directories(${PROJECT_SOURCE_DIR}/eo_ui.lv2)
include_directories(${PROJECT_SOURCE_DIR}/osc.lv2)
include_directories(${PROJECT_SOURCE_DIR}/timely.lv2)
include_directories(${PROJECT_SOURCE_DIR}/lua-5.3.1)
include_directories(${PROJECT_BINARY_DIR})

# check for visibility support in compiler on Unices
include(CheckCCompilerFlag)
if(NOT WIN32)
	check_c_compiler_flag(-fvisibility=hidden GCC_SUPPORTS_VISIBILITY)
	if(NOT ${GCC_SUPPORTS_VISIBILITY})
		message(FATAL_ERROR "compiler does not support visibility attributes")
	endif()
endif()

set(CMAKE_C_FLAGS "-std=gnu99 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,nodelete ${CMAKE_MODULE_LINKER_FLAGS}")
add_definitions("-D_GNU_SOURCE=1") # asprintf

set(MOONY_MAJOR_VERSION 0)
set(MOONY_MINOR_VERSION 1)
set(MOONY_MICRO_VERSION 1)
set(MOONY_VERSION "${MOONY_MAJOR_VERSION}.${MOONY_MINOR_VERSION}.${MOONY_MICRO_VERSION}")
add_definitions("-DMOONY_VERSION=\"${MOONY_VERSION}\"")

set(DEST lib/lv2/moony.lv2)
if(WIN32)
	set(LIB_EXT ".dll")
elseif(APPLE)
	set(LIB_EXT ".so")
else()
	set(LIB_EXT ".so")
endif()

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}
find_package(FLEX)

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

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

flex_target(encoder encoder.l ${PROJECT_BINARY_DIR}/encoder.c
	COMPILE_FLAGS	"--header-file=${PROJECT_BINARY_DIR}/encoder.h --prefix=enc")

add_library(moony MODULE
	moony.c
	api.c
	vm.c

	cxc.c
	axa.c
	caxca.c

	tlsf-3.0/tlsf.c

	lua-5.3.1/lapi.c
	lua-5.3.1/lcode.c
	lua-5.3.1/lctype.c
	lua-5.3.1/ldebug.c
	lua-5.3.1/ldo.c
	lua-5.3.1/ldump.c
	lua-5.3.1/lfunc.c
	lua-5.3.1/lgc.c
	lua-5.3.1/llex.c
	lua-5.3.1/lmem.c
	lua-5.3.1/lobject.c
	lua-5.3.1/lopcodes.c
	lua-5.3.1/lparser.c
	lua-5.3.1/lstate.c
	lua-5.3.1/lstring.c
	lua-5.3.1/ltable.c
	lua-5.3.1/ltm.c
	lua-5.3.1/lundump.c
	lua-5.3.1/lvm.c
	lua-5.3.1/lzio.c

	lua-5.3.1/lbaselib.c
	lua-5.3.1/lauxlib.c
	lua-5.3.1/lcorolib.c
	lua-5.3.1/lmathlib.c
	lua-5.3.1/lstrlib.c
	lua-5.3.1/ltablib.c
	lua-5.3.1/lutf8lib.c)
	#lua-5.3.1/lbitlib.c
	#lua-5.3.1/ldblib.c
	#lua-5.3.1/liolib.c
	#lua-5.3.1/loslib.c
	#lua-5.3.1/loadlib.c
	#lua-5.3.1/linit.c
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_compile_definitions(moony PUBLIC -DLUA_USE_LINUX)
elseif(WIN32)
	target_compile_definitions(moony PUBLIC)
elseif(APPLE)
	target_compile_definitions(moony PUBLIC -DLUA_USE_MACOSX)
endif()
target_link_libraries(moony ${LIBS} "-lm")
set_target_properties(moony PROPERTIES PREFIX "")
install(TARGETS moony DESTINATION ${DEST})

add_library(moony_ui MODULE
	moony_ui.c
	common_ui.c
	${FLEX_encoder_OUTPUTS})
target_link_libraries(moony_ui ${LIBS_UI})
set_target_properties(moony_ui PROPERTIES PREFIX "")
install(TARGETS moony_ui DESTINATION ${DEST})

configure_file(${PROJECT_SOURCE_DIR}/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/moony.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/presets.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/omk_logo_256x256.png DESTINATION ${DEST})

include(CTest)

if(${BUILD_TESTING})
	pkg_search_module(EINA REQUIRED eina>=1.8)
	include_directories(${EINA_INCLUDE_DIRS})

	add_executable(test_moony
		test_moony.c
		api.c
		vm.c

		ext_urid.c

		tlsf-3.0/tlsf.c

		lua-5.3.1/lapi.c
		lua-5.3.1/lcode.c
		lua-5.3.1/lctype.c
		lua-5.3.1/ldebug.c
		lua-5.3.1/ldo.c
		lua-5.3.1/ldump.c
		lua-5.3.1/lfunc.c
		lua-5.3.1/lgc.c
		lua-5.3.1/llex.c
		lua-5.3.1/lmem.c
		lua-5.3.1/lobject.c
		lua-5.3.1/lopcodes.c
		lua-5.3.1/lparser.c
		lua-5.3.1/lstate.c
		lua-5.3.1/lstring.c
		lua-5.3.1/ltable.c
		lua-5.3.1/ltm.c
		lua-5.3.1/lundump.c
		lua-5.3.1/lvm.c
		lua-5.3.1/lzio.c

		lua-5.3.1/lbaselib.c
		lua-5.3.1/lauxlib.c
		lua-5.3.1/lcorolib.c
		lua-5.3.1/lmathlib.c
		lua-5.3.1/lstrlib.c
		lua-5.3.1/ltablib.c
		lua-5.3.1/lutf8lib.c)
		#lua-5.3.1/lbitlib.c
		#lua-5.3.1/ldblib.c
		#lua-5.3.1/liolib.c
		#lua-5.3.1/loslib.c
		#lua-5.3.1/loadlib.c
		#lua-5.3.1/linit.c
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		target_compile_definitions(test_moony PUBLIC -DLUA_USE_LINUX)
	elseif(WIN32)
		target_compile_definitions(test_moony PUBLIC)
	elseif(APPLE)
		target_compile_definitions(test_moony PUBLIC -DLUA_USE_MACOSX)
	endif()
	target_link_libraries(test_moony ${EINA_LDFLAGS} "-lm")

	add_test(API-Test test_moony ${PROJECT_SOURCE_DIR}/test_moony.lua)
endif()

# CPack
set(CPACK_GENERATOR "TBZ2;DEB")

set(CPACK_PACKAGE_NAME "moony-lv2")
set(CPACK_PACKER_VERSION "0.1.0-Beta1")
set(CPACK_PACKAGE_CONTACT "Hanspeter Portner <dev@open-music-kontrollers.ch>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Realtime Lua as programmable glue in LV2")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
SET(CPACK_STRIP_FILES "lib/lv2/moony.lv2/moony.so;lib/lv2/moony.lv2/moon_ui.so")

# Debian
set(CPACK_DEBIAN_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
set(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_BUILD_DEPENDS "lv2core (>= 4.0-4~), lv2-extensions-good, libelementary-dev, flex")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libelementary2")
set(CPACK_DEBIAN_PACKAGE_PROVIDES "lv2-plugin")
set(CPACK_DEBIAN_PACKAGE_MAINAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://open-music-kontrollers.ch/lv2/moony/")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION_SUMMARY})
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")

include(CPack)

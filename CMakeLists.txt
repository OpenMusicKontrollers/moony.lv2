cmake_minimum_required(VERSION 2.8)

project(lua.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tlsf-3.0)
include_directories(${PROJECT_SOURCE_DIR}/libosc)
include_directories(${PROJECT_SOURCE_DIR}/eo_ui.lv2)
include_directories(${PROJECT_SOURCE_DIR}/lua.embed)
include_directories(${PROJECT_BINARY_DIR})

set(DEST lib/lv2/lua.lv2)
if(WIN32)
	set(LIB_EXT ".dll")
elseif(APPLE)
	set(LIB_EXT ".so")
else()
	set(LIB_EXT ".so")
endif()

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}
find_package(FLEX)
include (CheckCCompilerFlag)

# check for visibility support in compiler on Unices
if(NOT WIN32)
	check_c_compiler_flag(-fvisibility=hidden GCC_SUPPORTS_VISIBILITY)
	if(NOT ${GCC_SUPPORTS_VISIBILITY})
		message(FATAL_ERROR "compiler does not support visibility attributes")
	endif()
endif()

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

pkg_search_module(ELM REQUIRED elementary>=1.8)
include_directories(${ELM_INCLUDE_DIRS})
set(LIBS_UI ${LIBS_UI} ${ELM_LDFLAGS})

if((${ELM_VERSION} VERSION_EQUAL "1.13.0") OR (${ELM_VERSION} VERSION_GREATER "1.13.0"))
	add_definitions("-DELM_HAS_FAKE")
endif()

flex_target(encoder encoder.l ${PROJECT_BINARY_DIR}/encoder.c
	COMPILE_FLAGS	"--header-file=${PROJECT_BINARY_DIR}/encoder.h --prefix=enc")

add_library(lua MODULE
	lua_lv2.c
	lua_cxc.c
	lua_axa.c

	tlsf-3.0/tlsf.c

	libosc/osc.c

	lua.embed/lapi.c
	lua.embed/lcode.c
	lua.embed/lctype.c
	lua.embed/ldebug.c
	lua.embed/ldo.c
	lua.embed/ldump.c
	lua.embed/lfunc.c
	lua.embed/lgc.c
	lua.embed/llex.c
	lua.embed/lmem.c
	lua.embed/lobject.c
	lua.embed/lopcodes.c
	lua.embed/lparser.c
	lua.embed/lstate.c
	lua.embed/lstring.c
	lua.embed/ltable.c
	lua.embed/ltm.c
	lua.embed/lundump.c
	lua.embed/lvm.c
	lua.embed/lzio.c

	lua.embed/lbaselib.c
	lua.embed/lauxlib.c
	lua.embed/lcorolib.c
	lua.embed/lmathlib.c
	lua.embed/lstrlib.c
	lua.embed/ltablib.c
	lua.embed/lutf8lib.c)
	#lua.embed/lbitlib.c
	#lua.embed/ldblib.c
	#lua.embed/liolib.c
	#lua.embed/loslib.c
	#lua.embed/loadlib.c
	#lua.embed/linit.c
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_compile_definitions(lua PUBLIC -DLUA_USE_LINUX)
elseif(WIN32)
	target_compile_definitions(lua PUBLIC)
elseif(APPLE)
	target_compile_definitions(lua PUBLIC -DLUA_USE_MACOSX)
endif()
target_link_libraries(lua ${LIBS} "-lm")
set_target_properties(lua PROPERTIES PREFIX "")
install(TARGETS lua DESTINATION ${DEST})

option(BUILD_UI "Build UI" ON)
if(${BUILD_UI})
	add_library(lua_ui MODULE
		lua_lv2_ui.c
		lua_common_ui.c
		${FLEX_encoder_OUTPUTS})
	target_link_libraries(lua_ui ${LIBS_UI})
	set_target_properties(lua_ui PROPERTIES PREFIX "")
	install(TARGETS lua_ui DESTINATION ${DEST})
endif()

configure_file(${PROJECT_SOURCE_DIR}/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/lua.ttl DESTINATION ${DEST})

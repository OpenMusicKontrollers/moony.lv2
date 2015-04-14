cmake_minimum_required(VERSION 2.8)

project(lua.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tlsf-3.0)
include_directories(${PROJECT_SOURCE_DIR}/libosc)
include_directories(${PROJECT_SOURCE_DIR}/eo_ui.lv2)
include_directories(${PROJECT_BINARY_DIR})

set(DEST lib/lv2/lua.lv2)
set(LIB_EXT ".so")

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}
find_package(FLEX)

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

pkg_search_module(ELM REQUIRED elementary>=1.8)
include_directories(${ELM_INCLUDE_DIRS})
set(LIBS_UI ${LIBS_UI} ${ELM_LDFLAGS})

if((${ELM_VERSION} VERSION_EQUAL "1.13.0") OR (${ELM_VERSION} VERSION_GREATER "1.13.0"))
	add_definitions("-DELM_HAS_FAKE")
endif()

option(USE_LUAJIT "use LuaJIT" ON)
if(USE_LUAJIT)
	pkg_search_module(LUAJIT REQUIRED luajit>=2.0)
	include_directories(${LUAJIT_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${LUAJIT_LDFLAGS})
else()
	pkg_search_module(LUA REQUIRED lua>=5.1)
	include_directories(${LUA_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${LUA_LDFLAGS})
endif()

flex_target(encoder encoder.l ${PROJECT_BINARY_DIR}/encoder.c
	COMPILE_FLAGS	"--header-file=${PROJECT_BINARY_DIR}/encoder.h --prefix=enc")

add_library(lua MODULE
	lua_lv2.c
	tlsf-3.0/tlsf.c
	libosc/osc.c
	lua_control.c
	lua_atom.c)
target_link_libraries(lua ${LIBS})
set_target_properties(lua PROPERTIES PREFIX "")
install(TARGETS lua DESTINATION ${DEST})

add_library(lua_ui MODULE
	lua_lv2_ui.c
	lua_common_ui.c
	${FLEX_encoder_OUTPUTS})
target_link_libraries(lua_ui ${LIBS_UI})
set_target_properties(lua_ui PROPERTIES PREFIX "")
install(TARGETS lua_ui DESTINATION ${DEST})

configure_file(${PROJECT_SOURCE_DIR}/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/lua.ttl DESTINATION ${DEST})

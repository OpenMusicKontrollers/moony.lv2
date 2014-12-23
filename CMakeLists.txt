cmake_minimum_required(VERSION 2.8)

project(lua.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tlsf-3.0)
include_directories(${PROJECT_SOURCE_DIR}/libosc)

set(DEST lib/lv2/lua.lv2)
set(LIB_EXT ".so")

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

pkg_search_module(ELM REQUIRED elementary>=1.8)
include_directories(${ELM_INCLUDE_DIRS})
set(LIBS_UI ${LIBS_UI} ${ELM_LDFLAGS})

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

add_library(lua MODULE
	lua_lv2.c
	tlsf-3.0/tlsf.c
	libosc/osc.c
	lua_control.c
	lua_midi.c
	lua_osc.c)
target_link_libraries(lua ${LIBS})
set_target_properties(lua PROPERTIES PREFIX "")
install(TARGETS lua DESTINATION ${DEST})

add_library(lua_ui MODULE
	lua_lv2_ui.c
	lua_control_ui.c)
target_link_libraries(lua_ui ${LIBS_UI})
set_target_properties(lua_ui PROPERTIES PREFIX "")
install(TARGETS lua_ui DESTINATION ${DEST})

configure_file(${PROJECT_SOURCE_DIR}/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)

find_program(EDJE_CC NAMES edje_cc)
if(EDJE_CC_NOTFOUND)
  message(SEND_ERROR "edje_cc not found")
else()
  message(STATUS "edje_cc found: " ${EDJE_CC})
endif()

add_custom_command(
	OUTPUT ${PROJECT_BINARY_DIR}/lua.edj
	COMMAND ${EDJE_CC} ARGS
		-fd ${PROJECT_SOURCE_DIR}
		-id ${PROJECT_SOURCE_DIR}
		${PROJECT_SOURCE_DIR}/lua.edc
		${PROJECT_BINARY_DIR}/lua.edj
	MAIN_DEPENDENCY
		${PROJECT_SOURCE_DIR}/lua.edc
		${PROJECT_SOURCE_DIR}/lua_control.edc)
add_custom_target(THEME ALL DEPENDS ${PROJECT_BINARY_DIR}/lua.edj)

install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${DEST})
install(FILES ${PROJECT_BINARY_DIR}/lua.edj DESTINATION ${DEST})
install(FILES ${PROJECT_SOURCE_DIR}/lua.ttl DESTINATION ${DEST})

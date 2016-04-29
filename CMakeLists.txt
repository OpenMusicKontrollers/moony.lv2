cmake_minimum_required(VERSION 2.8)

project(moony.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tlsf-3.0)
include_directories(${PROJECT_SOURCE_DIR}/sandbox_ui.lv2)
include_directories(${PROJECT_SOURCE_DIR}/symap)
include_directories(${PROJECT_SOURCE_DIR}/osc.lv2)
include_directories(${PROJECT_SOURCE_DIR}/timely.lv2)
include_directories(${PROJECT_SOURCE_DIR}/xpress.lv2)
include_directories(${PROJECT_SOURCE_DIR}/lua-5.3.1)
include_directories(${PROJECT_SOURCE_DIR}/cJSON)
include_directories(${PROJECT_SOURCE_DIR}/api)
include_directories(${PROJECT_SOURCE_DIR}/ui)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugin)
include_directories(${PROJECT_BINARY_DIR})

# check for visibility support in compiler on Unices
include(CheckCCompilerFlag)
if(NOT WIN32)
	check_c_compiler_flag(-fvisibility=hidden GCC_SUPPORTS_VISIBILITY)
	if(NOT ${GCC_SUPPORTS_VISIBILITY})
		message(FATAL_ERROR "compiler does not support visibility attributes")
	endif()
endif()

set(CMAKE_C_FLAGS "-std=gnu11 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,defs ${CMAKE_MODULE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,nodelete ${CMAKE_MODULE_LINKER_FLAGS}")
endif()
add_definitions("-D_GNU_SOURCE=1") # asprintf

set(MOONY_MAJOR_VERSION 0)
set(MOONY_MINOR_VERSION 2)
set(MOONY_MICRO_VERSION 3)
set(MOONY_VERSION "${MOONY_MAJOR_VERSION}.${MOONY_MINOR_VERSION}.${MOONY_MICRO_VERSION}")
add_definitions("-DMOONY_VERSION=\"${MOONY_VERSION}\"")

if(NOT DEFINED PLUGIN_DEST)
	set(PLUGIN_DEST lib/lv2/moony.lv2)
endif()

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

# options
include(CMakeDependentOption)
option(BUILD_COMMON_UI "Build common embeddable UI" OFF)
option(BUILD_SIMPLE_UI "Build simple external UI" OFF)
option(BUILD_WEB_UI "Build web-based external UI" ON)
cmake_dependent_option(USE_FS_EVENT "Use filesystem notify events" ON "BUILD_SIMPLE_UI" OFF)

if(BUILD_COMMON_UI)
	pkg_search_module(ELM REQUIRED elementary>=1.8)
	include_directories(${ELM_INCLUDE_DIRS})

	pkg_search_module(NANOMSG REQUIRED libnanomsg>=2.0)
	include_directories(${NANOMSG_INCLUDE_DIRS})

	pkg_search_module(SRATOM REQUIRED sratom-0>=0.4.0)
	include_directories(${SRATOM_INCLUDE_DIRS})

	pkg_search_module(LILV REQUIRED lilv-0>=0.20.0)
	include_directories(${LILV_INCLUDE_DIRS})
	if((${LILV_VERSION} VERSION_EQUAL "0.22.0") OR (${LILV_VERSION} VERSION_GREATER "0.22.0"))
		add_definitions("-DLILV_0_22")
	endif()

	if((${ELM_VERSION} VERSION_EQUAL "1.9.0") OR (${ELM_VERSION} VERSION_GREATER "1.9.0"))
		add_definitions("-DELM_1_9")
	endif()

	find_package(FLEX)
	flex_target(encoder ${PROJECT_SOURCE_DIR}/ui/encoder.l ${PROJECT_BINARY_DIR}/encoder.c
		COMPILE_FLAGS	"--header-file=${PROJECT_BINARY_DIR}/encoder.h --prefix=enc")

	add_library(moony_ui MODULE
		${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_ui.c
		${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_master.c
		${PROJECT_SOURCE_DIR}/ui/moony_ui.c)
	target_link_libraries(moony_ui
		${NANOMSG_LDFLAGS}
		${SRATOM_LDFLAGS})
	set_target_properties(moony_ui PROPERTIES PREFIX "")
	install(TARGETS moony_ui DESTINATION ${PLUGIN_DEST})

	add_library(moony_eo MODULE
		${PROJECT_SOURCE_DIR}/ui/moony_eo.c
		${PROJECT_SOURCE_DIR}/ui/common_ui.c
		${FLEX_encoder_OUTPUTS})
	target_link_libraries(moony_eo
		${ELM_LDFLAGS})
	set_target_properties(moony_eo PROPERTIES PREFIX "")
	install(TARGETS moony_eo DESTINATION ${PLUGIN_DEST})

	add_executable(sandbox_efl
		${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_slave.c
		${PROJECT_SOURCE_DIR}/sandbox_ui.lv2/sandbox_efl.c
		${PROJECT_SOURCE_DIR}/symap/symap.c)
	target_link_libraries(sandbox_efl
		${ELM_LDFLAGS}
		${NANOMSG_LDFLAGS}
		${LILV_LDFLAGS})
	install(TARGETS sandbox_efl DESTINATION ${PLUGIN_DEST})


	install(FILES ${PROJECT_SOURCE_DIR}/ui/omk_logo_256x256.png DESTINATION ${PLUGIN_DEST})
else()
	set(COMMON_WRAP "#")
endif()

if(BUILD_SIMPLE_UI)
	if(USE_FS_EVENT)
		add_definitions("-DUSE_FS_EVENT=1")
	endif()

	pkg_search_module(UV REQUIRED libuv>=1.0)
	include_directories(${UV_INCLUDE_DIRS})
	if(DEFINED STATIC_UV)
		set(LIBS_SIMPLE ${STATIC_UV} ${LIBS_SIMPLE})
	else()
		set(LIBS_SIMPLE ${LIBS_SIMPLE} ${UV_LDFLAGS})
	endif()

	add_library(moony_simple MODULE
		ui/moony_simple.c
		ui/simple_ui.c)
	target_link_libraries(moony_simple ${LIBS_SIMPLE})
	set_target_properties(moony_simple PROPERTIES PREFIX "")
	install(TARGETS moony_simple DESTINATION ${PLUGIN_DEST})
else()
	set(SIMPLE_WRAP "#")
endif()

if(BUILD_WEB_UI)
	set(LIBS_WEB ${LIBS_WEB} m)

	pkg_search_module(LWS REQUIRED libwebsockets>=1.7.0)
	include_directories(${LWS_INCLUDE_DIRS})
	if(DEFINED STATIC_LWS)
		set(LIBS_WEB ${STATIC_LWS} ${LIBS_WEB})
	else()
		set(LIBS_WEB ${LIBS_WEB} ${LWS_LDFLAGS})
	endif()

	add_library(moony_web MODULE
		cJSON/cJSON.c
		ui/moony_web.c
		ui/web_ui.c)
	target_link_libraries(moony_web ${LIBS_WEB})
	set_target_properties(moony_web PROPERTIES PREFIX "")
	install(TARGETS moony_web DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/index.html DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/favicon.png DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/jquery-2.2.0.min.js DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/moony.js DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/style.css DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/Chango-Regular.ttf DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/ace.js DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/mode-lua.js DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/theme-chaos.js DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/keybinding-vim.js DESTINATION ${PLUGIN_DEST}/web_ui)
	install(FILES ${PROJECT_SOURCE_DIR}/web_ui/keybinding-emacs.js DESTINATION ${PLUGIN_DEST}/web_ui)
else()
	set(WEB_WRAP "#")
endif()

add_library(lua OBJECT
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

	#lua-5.3.1/lbitlib.c
	#lua-5.3.1/ldblib.c
	#lua-5.3.1/liolib.c
	#lua-5.3.1/loslib.c
	#lua-5.3.1/loadlib.c
	#lua-5.3.1/linit.c
	lua-5.3.1/lbaselib.c
	lua-5.3.1/lauxlib.c
	lua-5.3.1/lcorolib.c
	lua-5.3.1/lmathlib.c
	lua-5.3.1/lstrlib.c
	lua-5.3.1/ltablib.c
	lua-5.3.1/lutf8lib.c)
set_target_properties(lua PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC

add_library(api OBJECT
	api/api.c
	api/api_atom.c
	api/api_forge.c
	api/api_stash.c
	api/api_midi.c
	api/api_osc.c
	api/api_time.c
	api/api_state.c
	api/api_vm.c
	tlsf-3.0/tlsf.c)
set_target_properties(api PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC

add_library(moony MODULE
	plugin/moony.c
	plugin/cxc.c
	plugin/axa.c
	plugin/caxca.c
	$<TARGET_OBJECTS:api>
	$<TARGET_OBJECTS:lua>)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_compile_definitions(moony PUBLIC -DLUA_USE_LINUX)
elseif(WIN32)
	target_compile_definitions(moony PUBLIC)
elseif(APPLE)
	target_compile_definitions(moony PUBLIC -DLUA_USE_MACOSX)
endif()
target_link_libraries(moony ${LIBS} "-lm")
set_target_properties(moony PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto
set_target_properties(moony PROPERTIES PREFIX "")
install(TARGETS moony DESTINATION ${PLUGIN_DEST})

configure_file(${PROJECT_SOURCE_DIR}/plugin/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${PLUGIN_DEST})
install(FILES ${PROJECT_SOURCE_DIR}/plugin/moony.ttl DESTINATION ${PLUGIN_DEST})
install(FILES ${PROJECT_SOURCE_DIR}/plugin/moony_ui.ttl DESTINATION ${PLUGIN_DEST})
install(FILES ${PROJECT_SOURCE_DIR}/plugin/presets.ttl DESTINATION ${PLUGIN_DEST})

include(CTest)

if(${BUILD_TESTING})
	add_executable(moony_test
		test/moony_test.c
		$<TARGET_OBJECTS:api>
		$<TARGET_OBJECTS:lua>)
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		target_compile_definitions(moony_test PUBLIC -DLUA_USE_LINUX)
	elseif(WIN32)
		target_compile_definitions(moony_test PUBLIC)
	elseif(APPLE)
		target_compile_definitions(moony_test PUBLIC -DLUA_USE_MACOSX)
	endif()
	set_target_properties(moony_test PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto
	target_link_libraries(moony_test "-lm")

	if(NOT WIN32)
		add_test(NAME API-Test COMMAND moony_test ${PROJECT_SOURCE_DIR}/test/moony_test.lua)
	else()
		add_test(NAME API-Test COMMAND wine moony_test ${PROJECT_SOURCE_DIR}/test/moony_test.lua)
	endif()
endif()

# CPack
set(CPACK_GENERATOR "ZIP")

set(CPACK_PACKAGE_NAME "moony-lv2")
set(CPACK_PACKAGE_VERSION "${MOONY_VERSION}")
SET(CPACK_PACKAGE_VERSION_MAJOR ${MOONY_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${MOONY_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${MOONY_MICRO_VERSION})
set(CPACK_PACKAGE_CONTACT "Hanspeter Portner <dev@open-music-kontrollers.ch>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Realtime Lua as programmable glue in LV2")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
SET(CPACK_STRIP_FILES "lib/lv2/moony.lv2/moony.so;lib/lv2/moony.lv2/moon_ui.so;lib/lv2/moony.lv2/moony_simple.so;lib/lv2/moony.lv2/moony_web.so")

include(CPack)

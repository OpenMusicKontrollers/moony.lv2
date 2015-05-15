cmake_minimum_required(VERSION 2.8)

project(moony.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tlsf-3.0)
include_directories(${PROJECT_SOURCE_DIR}/eo_ui.lv2)
include_directories(${PROJECT_SOURCE_DIR}/lua-5.3)
include_directories(${PROJECT_BINARY_DIR})

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
include(CheckCCompilerFlag)

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

pkg_search_module(ECORE_X OPTIONAL ecore-x)

if((${ELM_VERSION} VERSION_EQUAL "1.9.0") OR (${ELM_VERSION} VERSION_GREATER "1.9.0"))
	add_definitions("-DELM_1_9")
endif()

if(${ECORE_X_FOUND} AND ((${ELM_VERSION} VERSION_EQUAL "1.13.0") OR (${ELM_VERSION} VERSION_GREATER "1.13.0")))
	set(X11_UI_WRAP "")
	add_definitions("-DX11_UI_WRAP")
else()
	set(X11_UI_WRAP "#")
endif()

flex_target(encoder encoder.l ${PROJECT_BINARY_DIR}/encoder.c
	COMPILE_FLAGS	"--header-file=${PROJECT_BINARY_DIR}/encoder.h --prefix=enc")

add_library(moony MODULE
	moony.c
	api.c
	vm.c

	cxc.c
	cxa.c
	axa.c
	axc.c
	caxca.c

	tlsf-3.0/tlsf.c

	lua-5.3/lapi.c
	lua-5.3/lcode.c
	lua-5.3/lctype.c
	lua-5.3/ldebug.c
	lua-5.3/ldo.c
	lua-5.3/ldump.c
	lua-5.3/lfunc.c
	lua-5.3/lgc.c
	lua-5.3/llex.c
	lua-5.3/lmem.c
	lua-5.3/lobject.c
	lua-5.3/lopcodes.c
	lua-5.3/lparser.c
	lua-5.3/lstate.c
	lua-5.3/lstring.c
	lua-5.3/ltable.c
	lua-5.3/ltm.c
	lua-5.3/lundump.c
	lua-5.3/lvm.c
	lua-5.3/lzio.c

	lua-5.3/lbaselib.c
	lua-5.3/lauxlib.c
	lua-5.3/lcorolib.c
	lua-5.3/lmathlib.c
	lua-5.3/lstrlib.c
	lua-5.3/ltablib.c
	lua-5.3/lutf8lib.c)
	#lua-5.3/lbitlib.c
	#lua-5.3/ldblib.c
	#lua-5.3/liolib.c
	#lua-5.3/loslib.c
	#lua-5.3/loadlib.c
	#lua-5.3/linit.c
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

		lua-5.3/lapi.c
		lua-5.3/lcode.c
		lua-5.3/lctype.c
		lua-5.3/ldebug.c
		lua-5.3/ldo.c
		lua-5.3/ldump.c
		lua-5.3/lfunc.c
		lua-5.3/lgc.c
		lua-5.3/llex.c
		lua-5.3/lmem.c
		lua-5.3/lobject.c
		lua-5.3/lopcodes.c
		lua-5.3/lparser.c
		lua-5.3/lstate.c
		lua-5.3/lstring.c
		lua-5.3/ltable.c
		lua-5.3/ltm.c
		lua-5.3/lundump.c
		lua-5.3/lvm.c
		lua-5.3/lzio.c

		lua-5.3/lbaselib.c
		lua-5.3/lauxlib.c
		lua-5.3/lcorolib.c
		lua-5.3/lmathlib.c
		lua-5.3/lstrlib.c
		lua-5.3/ltablib.c
		lua-5.3/lutf8lib.c)
		#lua-5.3/lbitlib.c
		#lua-5.3/ldblib.c
		#lua-5.3/liolib.c
		#lua-5.3/loslib.c
		#lua-5.3/loadlib.c
		#lua-5.3/linit.c
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

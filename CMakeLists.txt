cmake_minimum_required(VERSION 3.0)

project(moony.lv2)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tlsf-3.0)
include_directories(${PROJECT_SOURCE_DIR}/osc.lv2)
include_directories(${PROJECT_SOURCE_DIR}/timely.lv2)
include_directories(${PROJECT_SOURCE_DIR}/xpress.lv2)
include_directories(${PROJECT_SOURCE_DIR}/lua-5.3.4)
include_directories(${PROJECT_SOURCE_DIR}/tiny-AES128-C)
include_directories(${PROJECT_SOURCE_DIR}/laes128)
include_directories(${PROJECT_SOURCE_DIR}/api)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/plugin)
include_directories(${PROJECT_SOURCE_DIR}/pugl)
include_directories(${PROJECT_SOURCE_DIR}/varchunk)
include_directories(${PROJECT_BINARY_DIR})

# check for visibility support in compiler on Unices
include(CheckCCompilerFlag)
if(NOT WIN32)
	check_c_compiler_flag(-fvisibility=hidden GCC_SUPPORTS_VISIBILITY)
	if(NOT ${GCC_SUPPORTS_VISIBILITY})
		message(FATAL_ERROR "compiler does not support visibility attributes")
	endif()
endif()

set(CMAKE_C_FLAGS "-fdata-sections -ffunction-sections ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-std=gnu11 -Wextra -Wno-unused-parameter -ffast-math -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "-Wshadow -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wno-attributes ${CMAKE_C_FLAGS}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,defs ${CMAKE_MODULE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-z,nodelete ${CMAKE_MODULE_LINKER_FLAGS}")
elseif(WIN32)
	set(CMAKE_C_FLAGS "-mstackrealign ${CMAKE_C_FLAGS}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	if(APPLE)
		set(CMAKE_MODULE_LINKER_FLAGS "-Wl,-dead_strip ${CMAKE_MODULE_LINKER_FLAGS}")
	else()
		set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--gc-sections -Wl,-s ${CMAKE_MODULE_LINKER_FLAGS}")
	endif()
endif()

add_definitions("-D_GNU_SOURCE=1") # asprintf

file(STRINGS "VERSION" MOONY_VERSION)
string(REPLACE "." ";" VERSION_LIST ${MOONY_VERSION})
list(GET VERSION_LIST 0 MOONY_MAJOR_VERSION)
list(GET VERSION_LIST 1 MOONY_MINOR_VERSION)
list(GET VERSION_LIST 2 MOONY_MICRO_VERSION)
add_definitions("-DMOONY_MINOR_VERSION=${MOONY_MINOR_VERSION}")
add_definitions("-DMOONY_MICRO_VERSION=${MOONY_MICRO_VERSION}")
add_definitions("-DMOONY_VERSION=\"${MOONY_VERSION}\"")

if(NOT DEFINED PLUGIN_DEST)
	set(PLUGIN_DEST lib/lv2/moony.lv2)
endif()

find_package(PkgConfig) # ${PKG_CONFIG_FOUND}

set(LIBS ${LIBS} m)

pkg_search_module(LV2 REQUIRED lv2>=1.10)
include_directories(${LV2_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LV2_LDFLAGS})

# options
include(CMakeDependentOption)
option(BUILD_UI "Build UI" ON)
option(USE_MANUAL_GC "Use manual garbage collection" OFF)

if(USE_MANUAL_GC)
	add_definitions("-DUSE_MANUAL_GC")
endif()

if(BUILD_UI)
	set(LIBS_UI ${LIBS_UI} m)

	if(WIN32)
		set(MOONY_UI_TYPE "WindowsUI")
	elseif(APPLE)
		set(MOONY_UI_TYPE "CocoaUI")
	else()
		set(MOONY_UI_TYPE "X11UI")
	endif()

	find_package(OpenGL)
	if(${OPENGL_FOUND})
		set(LIBS_UI ${LIBS_UI} ${OPENGL_LIBRARIES})
	else() # try pkg-config
		pkg_search_module(GL REQUIRED gl)
		if(${GL_FOUND})
			set(LIBS_UI ${LIBS_UI} ${GL_LDFLAGS})
		else()
			message(FATAL_ERROR "OpenGL not found")
		endif()
	endif()
	add_definitions("-DPUGL_HAVE_GL")

	if(WIN32)
		find_library(GDI32_LIBRARY NAMES gdi32)
		if(GDI32_LIBRARY)
			set(LIBS_UI ${LIBS_UI} ${GDI32_LIBRARY})
		else()
			message(FATAL_ERROR "gdi32 library not found")
		endif()

		find_library(USER32_LIBRARY NAMES user32)
		if(USER32_LIBRARY)
			set(LIBS_UI ${LIBS_UI} ${USER32_LIBRARY})
		else()
			message(FATAL_ERROR "user32 library not found")
		endif()

		set(TAR_UI ${TAR_UI} pugl/pugl/pugl_win.cpp)

	elseif(APPLE)
		find_library(COCOA_LIBRARY NAMES Cocoa)
		if(COCOA_LIBRARY)
			set(LIBS_UI ${LIBS_UI} ${COCOA_LIBRARY})
		else()
			message(FATAL_ERROR "Cocoa framework not found")
		endif()

		set(TAR_UI ${TAR_UI} pugl/pugl/pugl_osx.m)

	else() # GNU/Linux
		pkg_search_module(X11 REQUIRED x11>=1.6)
		include_directories(${X11_INCLUDE_DIRS})
		set(LIBS_UI ${LIBS_UI} ${X11_LDFLAGS})

		pkg_search_module(XEXT REQUIRED xext>=1.3)
		include_directories(${XEXT_INCLUDE_DIRS})
		set(LIBS_UI ${LIBS_UI} ${XEXT_LDFLAGS})

		set(TAR_UI ${TAR_UI} pugl/pugl/pugl_x11.c)
	endif()

	add_library(moony_ui MODULE
		plugin/moony_nk.c
		plugin/nk_ui.c
		${TAR_UI}
		$<TARGET_OBJECTS:lua>
		$<TARGET_OBJECTS:lpeg>)
	target_link_libraries(moony_ui ${LIBS_UI})
	set_target_properties(moony_ui PROPERTIES PREFIX "")
	if(NOT WIN32)
		set_target_properties(moony_ui PROPERTIES LINK_FLAGS "-Wl,-e,lv2ui_descriptor")
	endif()
	install(TARGETS moony_ui DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/nuklear/extra_font/Cousine-Regular.ttf DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/plugin/lexer.lua DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/plugin/moony.lua DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/bell.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/cancel-1.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/cancel.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/checked.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/download.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/envelope.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/house.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/layers.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/menu.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/next.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/pencil.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/plus.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/question.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/reload.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/screen.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/settings.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/sort.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/upload.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/png/user.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/logo/omk_logo_256x256.png DESTINATION ${PLUGIN_DEST})
	install(FILES ${PROJECT_SOURCE_DIR}/logo/moony_logo.png DESTINATION ${PLUGIN_DEST})
else()
	set(UI_WRAP "#")
endif()

add_library(lua OBJECT
	lua-5.3.4/lapi.c
	lua-5.3.4/lcode.c
	lua-5.3.4/lctype.c
	lua-5.3.4/ldebug.c
	lua-5.3.4/ldo.c
	lua-5.3.4/ldump.c
	lua-5.3.4/lfunc.c
	lua-5.3.4/lgc.c
	lua-5.3.4/llex.c
	lua-5.3.4/lmem.c
	lua-5.3.4/lobject.c
	lua-5.3.4/lopcodes.c
	lua-5.3.4/lparser.c
	lua-5.3.4/lstate.c
	lua-5.3.4/lstring.c
	lua-5.3.4/ltable.c
	lua-5.3.4/ltm.c
	lua-5.3.4/lundump.c
	lua-5.3.4/lvm.c
	lua-5.3.4/lzio.c

	lua-5.3.4/liolib.c
	lua-5.3.4/loadlib.c
	#lua-5.3.4/lbitlib.c
	#lua-5.3.4/loslib.c
	#lua-5.3.4/linit.c

	lua-5.3.4/ldblib.c
	lua-5.3.4/lbaselib.c
	lua-5.3.4/lauxlib.c
	lua-5.3.4/lcorolib.c
	lua-5.3.4/lmathlib.c
	lua-5.3.4/lstrlib.c
	lua-5.3.4/ltablib.c
	lua-5.3.4/lutf8lib.c)
set_target_properties(lua PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC
set_target_properties(lua PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto

add_library(lpeg OBJECT
	lpeg-1.0.1/lpcap.c
	lpeg-1.0.1/lpcode.c
	lpeg-1.0.1/lpprint.c
	lpeg-1.0.1/lptree.c
	lpeg-1.0.1/lpvm.c)
set_target_properties(lpeg PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC
set_target_properties(lpeg PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto

add_library(lbase64 OBJECT
	lbase64/lbase64.c)
set_target_properties(lbase64 PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC
set_target_properties(lbase64 PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto

add_library(lascii85 OBJECT
	lascii85/lascii85.c)
set_target_properties(lascii85 PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC
set_target_properties(lascii85 PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto

add_library(laes128 OBJECT
	laes128/laes128.c
	tiny-AES128-C/aes.c)
set_target_properties(laes128 PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC
set_target_properties(laes128 PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto

add_library(api OBJECT
	api/api.c
	api/api_atom.c
	api/api_forge.c
	api/api_stash.c
	api/api_midi.c
	api/api_osc.c
	api/api_time.c
	api/api_state.c
	api/api_parameter.c
	api/api_vm.c
	tlsf-3.0/tlsf.c)
set_target_properties(api PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC
set_target_properties(api PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto

add_library(moony MODULE
	plugin/moony.c
	plugin/cxc.c
	plugin/axa.c
	plugin/caxca.c
	$<TARGET_OBJECTS:api>
	$<TARGET_OBJECTS:lua>
	$<TARGET_OBJECTS:lpeg>
	$<TARGET_OBJECTS:lbase64>
	$<TARGET_OBJECTS:lascii85>
	$<TARGET_OBJECTS:laes128>)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(moony PUBLIC -DLUA_USE_LINUX)
elseif(WIN32)
	target_compile_definitions(moony PUBLIC)
elseif(APPLE)
	target_compile_definitions(moony PUBLIC -DLUA_USE_MACOSX)
endif()
target_link_libraries(moony ${LIBS})
set_target_properties(moony PROPERTIES POSITION_INDEPENDENT_CODE true) # -fPIC
set_target_properties(moony PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto
set_target_properties(moony PROPERTIES PREFIX "")
if(NOT WIN32)
	set_target_properties(moony PROPERTIES LINK_FLAGS "-Wl,-e,lv2_descriptor")
endif()
install(TARGETS moony DESTINATION ${PLUGIN_DEST})

configure_file(${PROJECT_SOURCE_DIR}/plugin/manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
install(FILES ${PROJECT_BINARY_DIR}/manifest.ttl DESTINATION ${PLUGIN_DEST})
install(FILES ${PROJECT_SOURCE_DIR}/plugin/moony.ttl DESTINATION ${PLUGIN_DEST})
install(FILES ${PROJECT_SOURCE_DIR}/plugin/moony_ui.ttl DESTINATION ${PLUGIN_DEST})
install(FILES ${PROJECT_SOURCE_DIR}/plugin/presets.ttl DESTINATION ${PLUGIN_DEST})

include(CTest)

if(BUILD_TESTING)
	add_executable(moony_test
		test/moony_test.c
		$<TARGET_OBJECTS:api>
		$<TARGET_OBJECTS:lua>
		$<TARGET_OBJECTS:lpeg>
		$<TARGET_OBJECTS:lbase64>
		$<TARGET_OBJECTS:lascii85>
		$<TARGET_OBJECTS:laes128>)
	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		target_compile_definitions(moony_test PUBLIC -DLUA_USE_LINUX)
	elseif(WIN32)
		target_compile_definitions(moony_test PUBLIC)
	elseif(APPLE)
		target_compile_definitions(moony_test PUBLIC -DLUA_USE_MACOSX)
	endif()
	set_target_properties(moony_test PROPERTIES INTERPROCEDURAL_OPTIMIZATION true) # -flto
	target_link_libraries(moony_test ${LIBS})

	add_custom_command(
		OUTPUT ${PROJECT_BINARY_DIR}/manual.html
		COMMAND ${WINE} ${QEMU} ${PROJECT_BINARY_DIR}/moony_test
			${PROJECT_SOURCE_DIR}/manual/hilight.lua
			">"
			${PROJECT_BINARY_DIR}/manual.html
		DEPENDS
			moony_test
			${PROJECT_SOURCE_DIR}/plugin/lexer.lua
			${PROJECT_SOURCE_DIR}/plugin/moony.lua
			${PROJECT_SOURCE_DIR}/manual/hilight.lua
			${PROJECT_SOURCE_DIR}/manual/manual.html.in)
	add_custom_target(moony_manual ALL DEPENDS ${PROJECT_BINARY_DIR}/manual.html)
	install(FILES ${PROJECT_BINARY_DIR}/manual.html DESTINATION ${PLUGIN_DEST})

	add_test(NAME API-Test COMMAND ${WINE} ${QEMU} moony_test ${PROJECT_SOURCE_DIR}/test/moony_test.lua)
	add_test(NAME Manual-Test COMMAND ${WINE} ${QEMU} moony_test ${PROJECT_SOURCE_DIR}/test/moony_manual.lua)
	add_test(NAME Presets-Test COMMAND ${WINE} ${QEMU} moony_test ${PROJECT_SOURCE_DIR}/test/moony_presets.lua)
endif()

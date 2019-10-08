project('moony.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=1',
	'werror=false',
	'b_lto=false',
	'c_std=c11'])

build_opengl_ui = get_option('build-opengl-ui')
build_zimple_ui = get_option('build-zimple-ui')
build_canvas_ui = get_option('build-canvas-ui')
build_inline_disp = get_option('build-inline-disp')

static_link = meson.is_cross_build()

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.14.0')
cairo_dep = dependency('cairo', version : '>=1.15.0',
	required : build_inline_disp,
	static : static_link)

dsp_deps = [m_dep, lv2_dep, cairo_dep]
ui_deps = [m_dep, lv2_dep, cairo_dep]

if host_machine.system() == 'windows'
	# nothing to do for DSP
elif host_machine.system() == 'darwin'
	# nothing to do for DSP
else
	dsp_deps += cc.find_library('rt')
endif

lpeg_prefix = 'lpeg-1.0.2'
lua_prefix = 'lua-5.3.5'

canvas_inc = include_directories('canvas.lv2')
extui_inc = include_directories('ext_ui.lv2')
include_inc= include_directories('include')
api_inc = include_directories('api')
plugin_inc = include_directories('plugin')
laes128_inc = include_directories('laes128')
tinyaes_inc = include_directories('tiny-AES128-C')
lascii85_inc = include_directories('lascii85')
lbase64_inc = include_directories('lbase64')
lcomplex_inc = include_directories('lcomplex')
lmathx_inc = include_directories('lmathx')
lpeg_inc = include_directories(lpeg_prefix)
lrandom_inc = include_directories('lrandom')
lua_inc = include_directories(lua_prefix)
osc_inc = include_directories('osc.lv2')
pugl_inc = include_directories('pugl')
timely_inc = include_directories('timely.lv2')
tlsf_inc = include_directories('tlsf-3.0')
varchunk_inc = include_directories('varchunk')
xpress_inc = include_directories('xpress.lv2')
inc_dir = [canvas_inc, extui_inc, include_inc, api_inc, plugin_inc, laes128_inc, tinyaes_inc, lascii85_inc, lbase64_inc, lcomplex_inc, lmathx_inc, lpeg_inc, lrandom_inc, lua_inc, osc_inc, pugl_inc, timely_inc, tlsf_inc, varchunk_inc, xpress_inc]

inst_dir = join_paths(get_option('libdir'), 'lv2', meson.project_name())

rawvers = run_command('cat', 'VERSION').stdout().strip()
version = rawvers.split('.')

conf_data = configuration_data()
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

add_project_arguments('-DMOONY_VERSION="'+rawvers+'"', language : 'c')
add_project_arguments('-DMOONY_MINOR_VERSION='+version[1], language : 'c')
add_project_arguments('-DMOONY_MICRO_VERSION='+version[2], language : 'c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')
add_project_arguments('-DPUGL_HAVE_GL', language : 'c')
if build_inline_disp
	add_project_arguments('-DBUILD_INLINE_DISP', language : 'c')
endif

lv2_validate = find_program('lv2_validate', native : true, required : false)
sord_validate = find_program('sord_validate', native : true, required : false)
lv2lint = find_program('lv2lint', required : false)

c_args = ['-fvisibility=hidden',
	'-ffast-math',
	'-Wno-attributes',
	'-Wno-unused-function',
	'-Wno-unused-variable']

laes128_lib = static_library('laes128',
	join_paths('laes128', 'laes128.c'),
	join_paths('tiny-AES128-C', 'aes.c'),
	include_directories : inc_dir,
	c_args : c_args)

lascii85_lib = static_library('lascii85',
	join_paths('lascii85', 'lascii85.c'),
	include_directories : inc_dir,
	c_args : c_args)

lbase64_lib = static_library('lbase64',
	join_paths('lbase64', 'lbase64.c'),
	include_directories : inc_dir,
	c_args : c_args)

lcomplex_lib = static_library('lcomplex',
	join_paths('lcomplex', 'lcomplex.c'),
	include_directories : inc_dir,
	c_args : c_args)

lmathx_lib = static_library('lmathx',
	join_paths('lmathx', 'lmathx.c'),
	include_directories : inc_dir,
	c_args : c_args)

lrandom_lib = static_library('lrandom',
	join_paths('lrandom', 'lrandom.c'),
	include_directories : inc_dir,
	c_args : c_args)

lpeg_lib = static_library('lpeg',
	join_paths(lpeg_prefix, 'lpcap.c'),
	join_paths(lpeg_prefix, 'lpcode.c'),
	join_paths(lpeg_prefix, 'lpprint.c'),
	join_paths(lpeg_prefix, 'lptree.c'),
	join_paths(lpeg_prefix, 'lpvm.c'),
	include_directories : inc_dir,
	c_args : c_args)

lua_lib = static_library('lua',
	join_paths(lua_prefix, 'lapi.c'),
	join_paths(lua_prefix, 'lcode.c'),
	join_paths(lua_prefix, 'lctype.c'),
	join_paths(lua_prefix, 'ldebug.c'),
	join_paths(lua_prefix, 'ldo.c'),
	join_paths(lua_prefix, 'ldump.c'),
	join_paths(lua_prefix, 'lfunc.c'),
	join_paths(lua_prefix, 'lgc.c'),
	join_paths(lua_prefix, 'llex.c'),
	join_paths(lua_prefix, 'lmem.c'),
	join_paths(lua_prefix, 'lobject.c'),
	join_paths(lua_prefix, 'lopcodes.c'),
	join_paths(lua_prefix, 'lparser.c'),
	join_paths(lua_prefix, 'lstate.c'),
	join_paths(lua_prefix, 'lstring.c'),
	join_paths(lua_prefix, 'ltable.c'),
	join_paths(lua_prefix, 'ltm.c'),
	join_paths(lua_prefix, 'lundump.c'),
	join_paths(lua_prefix, 'lvm.c'),
	join_paths(lua_prefix, 'lzio.c'),

	join_paths(lua_prefix, 'liolib.c'),
	join_paths(lua_prefix, 'loadlib.c'),
	#join_paths(lua_prefix, 'lbitlib.c'),
	#join_paths(lua_prefix, 'loslib.c'),
	#join_paths(lua_prefix, 'linit.c'),

	join_paths(lua_prefix, 'ldblib.c'),
	join_paths(lua_prefix, 'lbaselib.c'),
	join_paths(lua_prefix, 'lauxlib.c'),
	join_paths(lua_prefix, 'lcorolib.c'),
	join_paths(lua_prefix, 'lmathlib.c'),
	join_paths(lua_prefix, 'lstrlib.c'),
	join_paths(lua_prefix, 'ltablib.c'),
	join_paths(lua_prefix, 'lutf8lib.c'),
	include_directories : inc_dir,
	c_args : c_args)

tlsf_lib = static_library('tlsf',
	join_paths('tlsf-3.0', 'tlsf.c'),
	include_directories : inc_dir,
	c_args : c_args)

api_lib = static_library('api',
	join_paths('api', 'api_atom.c'),
	join_paths('api', 'api.c'),
	join_paths('api', 'api_forge.c'),
	join_paths('api', 'api_midi.c'),
	join_paths('api', 'api_osc.c'),
	join_paths('api', 'api_parameter.c'),
	join_paths('api', 'api_stash.c'),
	join_paths('api', 'api_state.c'),
	join_paths('api', 'api_time.c'),
	join_paths('api', 'api_vm.c'),
	include_directories : inc_dir,
	dependencies : dsp_deps,
	c_args : c_args)

dsp_links = [laes128_lib, lascii85_lib, lbase64_lib, lcomplex_lib, lmathx_lib, lrandom_lib, lpeg_lib, lua_lib, tlsf_lib, api_lib]

ui_with = [lpeg_lib, lua_lib]
ui_links = []

dsp_srcs = [
	join_paths('plugin', 'moony.c'),
	join_paths('plugin', 'axa.c'),
	join_paths('plugin', 'caxca.c'),
	join_paths('plugin', 'cxc.c')]

app_srcs = [
	join_paths('test', 'moony_test.c')]

mod = shared_module('moony', dsp_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : dsp_deps,
	link_with : dsp_links,
	install : true,
	install_dir : inst_dir)

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)
conf_data.set('UI', '#')
conf_data.set('UI_TYPE', 'UI')

if build_inline_disp
	conf_data.set('BUILD_INLINE_DISPLAY', '')
else
	conf_data.set('BUILD_INLINE_DISPLAY', '#')
endif

if build_opengl_ui
	message('building OpenGL UI')
	conf_data.set('UI', '')
	conf_data.set('UI_OPENGL', '')
else
	conf_data.set('UI_OPENGL', '#')
endif

if build_zimple_ui
	message('building Zimple UI')
	conf_data.set('UI', '')
	conf_data.set('UI_ZIMPLE', '')
else
	conf_data.set('UI_ZIMPLE', '#')
endif

if build_canvas_ui
	message('building Canvas UI')
	conf_data.set('UI', '')
	conf_data.set('UI_CANVAS', '')
else
	conf_data.set('UI_CANVAS', '#')
endif

if build_opengl_ui or build_zimple_ui
	ui_srcs = [
		join_paths('plugin', 'moony_ui.c'),
		join_paths('plugin', 'simple_ui.c'),
		join_paths('plugin', 'nk_ui.c')]

	if host_machine.system() == 'windows'
		add_languages('cpp')
		conf_data.set('UI_TYPE', 'WindowsUI')
		ui_deps += cc.find_library('opengl32')
		ui_deps += cc.find_library('gdi32')
		ui_deps += cc.find_library('user32')
		ui_srcs += 'pugl/pugl/pugl_win.cpp'
	elif host_machine.system() == 'darwin'
		add_languages('objc')
		conf_data.set('UI_TYPE', 'CocoaUI')
		ui_links += ['-framework', 'OpenGL']
		ui_links += ['-framework', 'Cocoa']
		ui_srcs += 'pugl/pugl/pugl_osx.m'
	else
		conf_data.set('UI_TYPE', 'X11UI')
		dsp_deps += cc.find_library('rt')
		ui_deps += dependency('gl')
		ui_deps += dependency('x11', version : '>=1.6.0')
		ui_deps += dependency('xext', version : '>=1.3.0')
		ui_srcs += 'pugl/pugl/pugl_x11.c'
	endif

	ui = shared_module('moony_ui', ui_srcs,
		c_args : c_args,
		include_directories : inc_dir,
		name_prefix : '',
		dependencies : ui_deps,
		link_with : ui_with,
		link_args : ui_links,
		install : true,
		install_dir : inst_dir)
endif

app = executable('moony_test', app_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : dsp_deps,
	link_with : dsp_links,
	install : false)

manifest_ttl = configure_file(
	input : join_paths('plugin', 'manifest.ttl.in'),
	output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)

dsp_ttl = configure_file(
	input : join_paths('plugin', 'moony.ttl.in'),
	output : 'moony.ttl',
	configuration: conf_data,
	install : true,
	install_dir : inst_dir)

ui_ttl = configure_file(
	input : join_paths('plugin', 'moony_ui.ttl'),
	output : 'moony_ui.ttl',
	copy : true,
	install : true,
	install_dir : inst_dir)

pset_ttl = configure_file(
	input : join_paths('plugin', 'presets.ttl'),
	output : 'presets.ttl',
	copy : true,
	install : true,
	install_dir : inst_dir)

configure_file(
	input : join_paths('nuklear', 'extra_font', 'Cousine-Regular.ttf'),
	output : 'Cousine-Regular.ttf',
	copy : true,
	install : true,
	install_dir : inst_dir)

lexer_lua = configure_file(
	input : join_paths('plugin', 'lexer.lua'),
	output : 'lexer.lua',
	copy : true,
	install : true,
	install_dir : inst_dir)
moony_lua = configure_file(
	input : join_paths('plugin', 'moony.lua'),
	output : 'moony.lua',
	copy : true,
	install : true,
	install_dir : inst_dir)

configure_file(
	input : join_paths('png', 'bell.png'),
	output : 'bell.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'cancel-1.png'),
	output : 'cancel-1.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'cancel.png'),
	output : 'cancel.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'checked.png'),
	output : 'checked.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'download.png'),
	output : 'download.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'envelope.png'),
	output : 'envelope.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'house.png'),
	output : 'house.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'layers.png'),
	output : 'layers.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'menu.png'),
	output : 'menu.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'next.png'),
	output : 'next.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'pencil.png'),
	output : 'pencil.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'plus.png'),
	output : 'plus.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'question.png'),
	output : 'question.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'reload.png'),
	output : 'reload.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'screen.png'),
	output : 'screen.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'settings.png'),
	output : 'settings.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'sort.png'),
	output : 'sort.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'upload.png'),
	output : 'upload.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('png', 'user.png'),
	output : 'user.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('logo', 'omk_logo_256x256.png'),
	output : 'omk_logo_256x256.png',
	copy : true,
	install : true,
	install_dir : inst_dir)
configure_file(
	input : join_paths('logo', 'moony_logo.png'),
	output : 'moony_logo.png',
	copy : true,
	install : true,
	install_dir : inst_dir)

moony_test_lua = configure_file(
	input : join_paths('test', 'moony_test.lua'),
	output : 'moony_test.lua',
	copy : true,
	install : false)
moony_overflow_lua = configure_file(
	input : join_paths('test', 'moony_overflow.lua'),
	output : 'moony_overflow.lua',
	copy : true,
	install : false)
moony_manual_lua = configure_file(
	input : join_paths('test', 'moony_manual.lua'),
	output : 'moony_manual.lua',
	copy : true,
	install : false)
moony_presets_lua = configure_file(
	input : join_paths('test', 'moony_presets.lua'),
	output : 'moony_presets.lua',
	copy : true,
	install : false)

manual_html_in = configure_file(
	input : join_paths('manual', 'manual.html.in'),
	output : 'manual.html.in',
	copy: true,
	install : false)
hilight_lua = configure_file(
	input : join_paths('manual', 'hilight.lua'),
	output : 'hilight.lua',
	copy : true,
	install : false)

if host_machine.system() != 'darwin'
	custom_target('manual_html',
		input : hilight_lua,
		output : 'manual.html',
		capture : true,
		command : [app, '@INPUT@'],
		#depends : [lexer_lua, moony_lua, manual_html_in], FIXME
		install : true,
		install_dir : inst_dir)

	test('API', app,
		args : [moony_test_lua])
	test('Overflow', app,
		args : [moony_overflow_lua, '0'])
	test('Manual', app,
		args : [moony_manual_lua])
	test('Presets', app,
		args : [moony_presets_lua])
endif

if lv2_validate.found() and sord_validate.found()
	test('LV2 validate', lv2_validate,
		args : [manifest_ttl, dsp_ttl, ui_ttl, pset_ttl])
endif

if lv2lint.found()
	test('LV2 lint', lv2lint,
		args : ['-Ewarn',
			'http://open-music-kontrollers.ch/lv2/moony#a1xa1',
			'http://open-music-kontrollers.ch/lv2/moony#a2xa2',
			'http://open-music-kontrollers.ch/lv2/moony#a4xa4',
			'http://open-music-kontrollers.ch/lv2/moony#c1a1xc1a1',
			'http://open-music-kontrollers.ch/lv2/moony#c1xc1',
			'http://open-music-kontrollers.ch/lv2/moony#c2a1xc2a1',
			'http://open-music-kontrollers.ch/lv2/moony#c2xc2',
			'http://open-music-kontrollers.ch/lv2/moony#c4a1xc4a1',
			'http://open-music-kontrollers.ch/lv2/moony#c4xc4'])
endif

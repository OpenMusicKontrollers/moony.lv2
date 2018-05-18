project('moony.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=1',
	'werror=false',
	'b_lto=false',
	'c_std=c11'])

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.14.0')

dsp_deps = [m_dep, lv2_dep]
ui_deps = [m_dep, lv2_dep]

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
lpeg_inc = include_directories('lpeg-1.0.1')
lrandom_inc = include_directories('lrandom')
lua_inc = include_directories('lua-5.3.4')
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

cp = find_program('cp')
clone = [cp, '@INPUT@', '@OUTPUT@']

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
	join_paths('lpeg-1.0.1', 'lpcap.c'),
	join_paths('lpeg-1.0.1', 'lpcode.c'),
	join_paths('lpeg-1.0.1', 'lpprint.c'),
	join_paths('lpeg-1.0.1', 'lptree.c'),
	join_paths('lpeg-1.0.1', 'lpvm.c'),
	include_directories : inc_dir,
	c_args : c_args)

lua_lib = static_library('lua',
	join_paths('lua-5.3.4', 'lapi.c'),
	join_paths('lua-5.3.4', 'lcode.c'),
	join_paths('lua-5.3.4', 'lctype.c'),
	join_paths('lua-5.3.4', 'ldebug.c'),
	join_paths('lua-5.3.4', 'ldo.c'),
	join_paths('lua-5.3.4', 'ldump.c'),
	join_paths('lua-5.3.4', 'lfunc.c'),
	join_paths('lua-5.3.4', 'lgc.c'),
	join_paths('lua-5.3.4', 'llex.c'),
	join_paths('lua-5.3.4', 'lmem.c'),
	join_paths('lua-5.3.4', 'lobject.c'),
	join_paths('lua-5.3.4', 'lopcodes.c'),
	join_paths('lua-5.3.4', 'lparser.c'),
	join_paths('lua-5.3.4', 'lstate.c'),
	join_paths('lua-5.3.4', 'lstring.c'),
	join_paths('lua-5.3.4', 'ltable.c'),
	join_paths('lua-5.3.4', 'ltm.c'),
	join_paths('lua-5.3.4', 'lundump.c'),
	join_paths('lua-5.3.4', 'lvm.c'),
	join_paths('lua-5.3.4', 'lzio.c'),

	join_paths('lua-5.3.4', 'liolib.c'),
	join_paths('lua-5.3.4', 'loadlib.c'),
	#join_paths('lua-5.3.4', 'lbitlib.c'),
	#join_paths('lua-5.3.4', 'loslib.c'),
	#join_paths('lua-5.3.4', 'linit.c'),

	join_paths('lua-5.3.4', 'ldblib.c'),
	join_paths('lua-5.3.4', 'lbaselib.c'),
	join_paths('lua-5.3.4', 'lauxlib.c'),
	join_paths('lua-5.3.4', 'lcorolib.c'),
	join_paths('lua-5.3.4', 'lmathlib.c'),
	join_paths('lua-5.3.4', 'lstrlib.c'),
	join_paths('lua-5.3.4', 'ltablib.c'),
	join_paths('lua-5.3.4', 'lutf8lib.c'),
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

ui_links = [lpeg_lib, lua_lib]

dsp_srcs = [
	join_paths('plugin', 'moony.c'),
	join_paths('plugin', 'axa.c'),
	join_paths('plugin', 'caxca.c'),
	join_paths('plugin', 'cxc.c')]

app_srcs = [
	join_paths('test', 'moony_test.c')]

ui_srcs = [
	join_paths('plugin', 'moony_ui.c'),
	join_paths('plugin', 'simple_ui.c'),
	join_paths('plugin', 'nk_ui.c')]

if host_machine.system() == 'linux'
	conf_data.set('UI_TYPE', 'X11UI')
	dsp_deps += cc.find_library('rt')
	ui_deps += dependency('gl')
	ui_deps += dependency('x11', version : '>=1.6.0')
	ui_deps += dependency('xext', version : '>=1.3.0')
	ui_srcs += 'pugl/pugl/pugl_x11.c'
elif host_machine.system() == 'windows'
	add_languages('cpp')
	conf_data.set('UI_TYPE', 'WindowsUI')
	ui_deps += cc.find_library('opengl32')
	ui_deps += cc.find_library('gdi32')
	ui_deps += cc.find_library('user32')
	ui_srcs += 'pugl/pugl/pugl_win.cpp'
elif host_machine.system() == 'darwin'
	#add_languages('objc')
	conf_data.set('UI_TYPE', 'CocoaUI')
	#ui_deps += cc.find_library('Cocoa')
	#ui_deps += cc.find_library('gl')
	#ui_deps += dependency('appleframeworks', modules : 'cocoa')
	#ui_srcs += 'pugl/pugl/pugl_osx.m'
endif

mod = shared_module('moony', dsp_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : dsp_deps,
	link_with : dsp_links,
	install : true,
	install_dir : inst_dir)

ui = shared_module('moony_ui', ui_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : ui_deps,
	link_with : ui_links,
	install : true,
	install_dir : inst_dir)

app = executable('moony_test', app_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : dsp_deps,
	link_with : dsp_links,
	install : false)

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)

conf_data.set('UI_EXT', '') #FIXME
conf_data.set('UI_WRAP', '') #FIXME

configure_file(
	input : join_paths('plugin', 'manifest.ttl.in'),
	output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)

custom_target('dsp_ttl',
	input : join_paths('plugin', 'moony.ttl'),
	output : 'moony.ttl',
	command : clone,
	install : true,
	install_dir : inst_dir)

custom_target('ui_ttl',
	input : join_paths('plugin', 'moony_ui.ttl'),
	output : 'moony_ui.ttl',
	command : clone,
	install : true,
	install_dir : inst_dir)

custom_target('presets_ttl',
	input : join_paths('plugin', 'presets.ttl'),
	output : 'presets.ttl',
	command : clone,
	install : true,
	install_dir : inst_dir)

custom_target('font',
	input : join_paths('nuklear', 'extra_font', 'Cousine-Regular.ttf'),
	output : 'Cousine-Regular.ttf',
	command : clone,
	install : true,
	install_dir : inst_dir)

lexer_lua = custom_target('lexer_lua',
	input : join_paths('plugin', 'lexer.lua'),
	output : 'lexer.lua',
	command : clone,
	install : true,
	install_dir : inst_dir)
moony_lua = custom_target('moony_lua',
	input : join_paths('plugin', 'moony.lua'),
	output : 'moony.lua',
	command : clone,
	install : true,
	install_dir : inst_dir)

custom_target('bell_png',
	input : join_paths('png', 'bell.png'),
	output : 'bell.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('cancel-1_png',
	input : join_paths('png', 'cancel-1.png'),
	output : 'cancel-1.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('cancel_png',
	input : join_paths('png', 'cancel.png'),
	output : 'cancel.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('checked_png',
	input : join_paths('png', 'checked.png'),
	output : 'checked.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('download_png',
	input : join_paths('png', 'download.png'),
	output : 'download.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('envelope_png',
	input : join_paths('png', 'envelope.png'),
	output : 'envelope.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('house_png',
	input : join_paths('png', 'house.png'),
	output : 'house.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('layers_png',
	input : join_paths('png', 'layers.png'),
	output : 'layers.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('menu_png',
	input : join_paths('png', 'menu.png'),
	output : 'menu.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('next_png',
	input : join_paths('png', 'next.png'),
	output : 'next.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('pencil_png',
	input : join_paths('png', 'pencil.png'),
	output : 'pencil.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('plus_png',
	input : join_paths('png', 'plus.png'),
	output : 'plus.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('question_png',
	input : join_paths('png', 'question.png'),
	output : 'question.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('reload_png',
	input : join_paths('png', 'reload.png'),
	output : 'reload.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('screen_png',
	input : join_paths('png', 'screen.png'),
	output : 'screen.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('settings_png',
	input : join_paths('png', 'settings.png'),
	output : 'settings.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('sort_png',
	input : join_paths('png', 'sort.png'),
	output : 'sort.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('upload_png',
	input : join_paths('png', 'upload.png'),
	output : 'upload.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('user_png',
	input : join_paths('png', 'user.png'),
	output : 'user.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('omk_logo_256x256_png',
	input : join_paths('logo', 'omk_logo_256x256.png'),
	output : 'omk_logo_256x256.png',
	command : clone,
	install : true,
	install_dir : inst_dir)
custom_target('moony_logo_png',
	input : join_paths('logo', 'moony_logo.png'),
	output : 'moony_logo.png',
	command : clone,
	install : true,
	install_dir : inst_dir)

moony_test_lua = custom_target('moony_test_lua',
	input : join_paths('test', 'moony_test.lua'),
	output : 'moony_test.lua',
	command : clone,
	install : false)
moony_overflow_lua = custom_target('moony_overflow_lua',
	input : join_paths('test', 'moony_overflow.lua'),
	output : 'moony_overflow.lua',
	command : clone,
	install : false)
moony_manual_lua = custom_target('moony_manual_lua',
	input : join_paths('test', 'moony_manual.lua'),
	output : 'moony_manual.lua',
	command : clone,
	install : false)
moony_presets_lua = custom_target('moony_presets_lua',
	input : join_paths('test', 'moony_presets.lua'),
	output : 'moony_presets.lua',
	command : clone,
	install : false)

manual_html_in = custom_target('manual_html_in',
	input : join_paths('manual', 'manual.html.in'),
	output : 'manual.html.in',
	command : clone,
	install : false)
hilight_lua = custom_target('hilight_lua',
	input : join_paths('manual', 'hilight.lua'),
	output : 'hilight.lua',
	command : clone,
	install : false)

if host_machine.system() != 'darwin'
	custom_target('manual_html',
		input : hilight_lua,
		output : 'manual.html',
		capture : true,
		command : [app, '@INPUT@'],
		depends : [lexer_lua, moony_lua, manual_html_in],
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

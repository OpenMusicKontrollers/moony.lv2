project('moony.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=1',
	'werror=false',
	'b_lto=false',
	'c_std=gnu11'])

d2tk = subproject('d2tk')
nk_pugl = subproject('nk_pugl')

lv2libdir = get_option('lv2libdir')
build_tests = get_option('build-tests')

inst_dir = join_paths(lv2libdir, meson.project_name())

if get_option('use-backend-nanovg').enabled()
	d2tk_dep = d2tk.get_variable('d2tk_nanovg')
elif get_option('use-backend-cairo').enabled()
	d2tk_dep = d2tk.get_variable('d2tk_cairo')
else
	error('no valid UI backend given')
endif

nk_pugl_dep = nk_pugl.get_variable('nk_pugl_gl')
cousine_regular_ttf = nk_pugl.get_variable('cousine_regular_ttf')

source_root = meson.source_root()
build_root = meson.build_root()

build_opengl_ui = get_option('build-opengl-ui')
build_next_ui = get_option('build-next-ui')
build_inline_disp = get_option('build-inline-disp')

static_link = meson.is_cross_build()

cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.14.0')
cairo_dep = dependency('cairo', version : '>=1.15.0',
	required : build_inline_disp,
	static : static_link)
thread_dep = dependency('threads')

dsp_deps = [m_dep, lv2_dep, cairo_dep, thread_dep]
nk_ui_deps = [m_dep, lv2_dep, cairo_dep, thread_dep, nk_pugl_dep]
d2tk_ui_deps = [m_dep, lv2_dep, d2tk_dep]

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
timely_inc = include_directories('timely.lv2')
tlsf_inc = include_directories('tlsf-3.0')
varchunk_inc = include_directories('varchunk')
xpress_inc = include_directories('xpress.lv2')
nk_pugl_inc = include_directories(join_paths('subprojects', 'nk_pugl'))
d2tk_inc = include_directories(join_paths('subprojects', 'd2tk'))
props_inc = include_directories('props.lv2')
ser_atom_inc = include_directories('ser_atom.lv2')
inc_dir = [canvas_inc, extui_inc, include_inc, api_inc, plugin_inc, laes128_inc, tinyaes_inc, lascii85_inc, lbase64_inc, lcomplex_inc, lmathx_inc, lpeg_inc, lrandom_inc, lua_inc, osc_inc, timely_inc, tlsf_inc, varchunk_inc, xpress_inc, nk_pugl_inc, d2tk_inc, props_inc, ser_atom_inc]

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
if build_inline_disp
	add_project_arguments('-DBUILD_INLINE_DISP', language : 'c')
endif

lv2_validate = find_program('lv2_validate', native : true, required : false)
sord_validate = find_program('sord_validate', native : true, required : false)
lv2lint = find_program('lv2lint', required : false)

c_args = [
	'-fvisibility=hidden',
	'-Wno-attributes',
	'-Wno-unused-function']

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

if build_next_ui
	message('building Simple UI')
	conf_data.set('UI', '')
	conf_data.set('UI_NEXT', '')
else
	conf_data.set('UI_NEXT', '#')
endif

if host_machine.system() == 'windows'
	conf_data.set('UI_TYPE', 'WindowsUI')
elif host_machine.system() == 'darwin'
	conf_data.set('UI_TYPE', 'CocoaUI')
else
	conf_data.set('UI_TYPE', 'X11UI')
endif

if build_opengl_ui
	nk_ui_srcs = [
		join_paths('plugin', 'nk_ui.c')]

	nk_ui = shared_module('moony_ui', nk_ui_srcs,
		c_args : c_args,
		include_directories : inc_dir,
		name_prefix : '',
		dependencies : nk_ui_deps,
		link_with : ui_with,
		install : true,
		install_dir : inst_dir)
endif

if build_next_ui
	d2tk_ui_srcs = [
		join_paths('plugin', 'd2tk_ui.c')]

	d2tk_ui = shared_module('moony_d2tk', d2tk_ui_srcs,
		c_args : c_args,
		include_directories : inc_dir,
		name_prefix : '',
		dependencies : d2tk_ui_deps,
		install : true,
		install_dir : inst_dir)

	use_fontconfig = get_option('use-fontconfig')
	if not use_fontconfig.enabled()
		fira_sans_bold_ttf = d2tk.get_variable('fira_sans_bold_ttf')
		fira_code_bold_ttf = d2tk.get_variable('fira_code_bold_ttf')
		fira_code_light_ttf = d2tk.get_variable('fira_code_light_ttf')
		fira_code_medium_ttf = d2tk.get_variable('fira_code_medium_ttf')
		fira_code_regular_ttf = d2tk.get_variable('fira_code_regular_ttf')

		configure_file(
			input : fira_sans_bold_ttf,
			output : 'FiraSans:bold.ttf',
			copy : true,
			install : true,
			install_dir : inst_dir)
		configure_file(
			input : fira_code_bold_ttf,
			output : 'FiraCode:bold.ttf',
			copy : true,
			install : true,
			install_dir : inst_dir)
		configure_file(
			input : fira_code_light_ttf,
			output : 'FiraCode:light.ttf',
			copy : true,
			install : true,
			install_dir : inst_dir)
		configure_file(
			input : fira_code_medium_ttf,
			output : 'FiraCode:medium.ttf',
			copy : true,
			install : true,
			install_dir : inst_dir)
		configure_file(
			input : fira_code_regular_ttf,
			output : 'FiraCode:regular.ttf',
			copy : true,
			install : true,
			install_dir : inst_dir)
	endif
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
	input : cousine_regular_ttf,
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
	input : join_paths('png', 'libre-gui-exclamation-circle.png'),
	output : 'libre-gui-exclamation-circle.png',
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
		args : [ '-I', join_paths(build_root, ''),
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

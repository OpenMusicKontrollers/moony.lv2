project('xpress.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=true',
	'b_lto=false',
	'c_std=c11'])

add_project_arguments('-D_GNU_SOURCE', language : 'c')

conf_data = configuration_data()
cc = meson.get_compiler('c')

cp = find_program('cp')
lv2_validate = find_program('lv2_validate', native : true, required : false)
sord_validate = find_program('sord_validate', native : true, required : false)
lv2lint = find_program('lv2lint', required : false)
clone = [cp, '@INPUT@', '@OUTPUT@']

lv2_dep = dependency('lv2', version : '>=1.14.0')
deps = [lv2_dep]

inc_dir = []

inst_dir = join_paths(get_option('libdir'), 'lv2', meson.project_name())

dsp_srcs = [join_paths('test', 'xpress.c')]

c_args = ['-fvisibility=hidden',
	'-ffast-math']

if host_machine.system() == 'linux'
	rt_dep = cc.find_library('rt')
	deps += rt_dep
endif

mod = shared_module('xpress', dsp_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : deps,
	install : true,
	install_dir : inst_dir)

version = run_command('cat', 'VERSION').stdout().strip().split('.')
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)

manifest_ttl = configure_file(input : join_paths('test', 'manifest.ttl.in'),
	output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)
dsp_ttl = custom_target('xpress_ttl',
	input : join_paths('test', 'xpress.ttl'),
	output : 'xpress.ttl',
	command : clone,
	install : true,
	install_dir : inst_dir)

xpress_test = executable('xpress_test',
	join_paths('test', 'xpress_test.c'),
	c_args : c_args,
	dependencies : deps,
	install : false)

test('Test', xpress_test,
	timeout : 240)

if lv2_validate.found() and sord_validate.found()
	test('LV2 validate', lv2_validate,
		args : [manifest_ttl, dsp_ttl])
endif

if lv2lint.found()
	test('LV2 lint', lv2lint,
		args : ['-Ewarn',
			'http://open-music-kontrollers.ch/lv2/xpress#test'])
endif

project('xpress.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=1',
	'werror=false',
	'b_lto=true',
	'c_std=c11'])

add_project_arguments('-D_GNU_SOURCE', language : 'c')

conf_data = configuration_data()
cc = meson.get_compiler('c')

cp = find_program('cp')
clone = [cp, '@INPUT@', '@OUTPUT@']

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.14.0')

inc_dir = []

inst_dir = join_paths(get_option('libdir'), 'lv2', meson.project_name())

dsp_srcs = [join_paths('test', 'xpress.c')]

c_args = ['-fvisibility=hidden',
	'-ffast-math']

mod = shared_module('xpress', dsp_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : [m_dep, lv2_dep],
	install : true,
	install_dir : inst_dir)

version = run_command('cat', 'VERSION').stdout().strip().split('.')
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)

configure_file(input : join_paths('test', 'manifest.ttl.in'), output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)
custom_target('xpress_ttl',
	input : join_paths('test', 'xpress.ttl'),
	output : 'xpress.ttl',
	command : clone,
	install : true,
	install_dir : inst_dir)

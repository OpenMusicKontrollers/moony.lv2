project('ser_atom.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=true',
	'b_lto=false',
	'c_std=c11'])

version = run_command('cat', 'VERSION').stdout().strip()

add_project_arguments('-D_GNU_SOURCE', language : 'c')

conf_data = configuration_data()
cc = meson.get_compiler('c')

lv2_dep = dependency('lv2')
deps = [lv2_dep]

c_args = []

ser_atom_test = executable('ser_atom_test',
	join_paths('test', 'ser_atom_test.c'),
	c_args : c_args,
	dependencies : deps,
	install : false)

test('Test', ser_atom_test)

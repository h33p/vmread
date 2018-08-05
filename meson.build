project('vctk', 'cpp', 'c', default_options : ['c_std=c99'])

compile_args = ['-pedantic', '-Werror']

dl = meson.get_compiler('c').find_library('dl', required : false)
thread = meson.get_compiler('c').find_library('pthread', required : false)

example = executable(
	'example',
	files('example.c', 'mem.c', 'vmmem.c', 'wintools.c'),
  c_args : compile_args
)

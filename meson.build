project('vctk', 'cpp', 'c', default_options : ['c_std=c99'])

compile_args = ['-pedantic', '-Werror', '-DMVERBOSE=4', '-fsanitize=address', '-lasan', '-fmax-errors=10', '-DREAD_CHECK']

dl = meson.get_compiler('c').find_library('dl', required : false)
thread = meson.get_compiler('c').find_library('pthread', required : false)

base_files = ['mem.c', 'wintools.c']

example = executable(
	'example',
	files(base_files + ['example.c', 'vmmem.c']),
  c_args : compile_args + ['-DLMODE=MODE_EXTERNAL'],
  link_args : compile_args
)

example_lib = shared_library(
	'example',
	files(base_files + ['example.c', 'intmem.c']),
  c_args : ['-DLMODE=MODE_QEMU_INJECT', '-DMVERBOSE=4'],
  dependencies: [dl]
)

project('vctk', 'cpp', 'c', default_options : ['c_std=c99', 'c++_std=c++11'])

compile_args = ['-pedantic', '-Werror', '-Wno-sign-conversion', '-Wno-c++98-compat', '-Wno-padded', '-Wno-c++98-compat-pedantic', '-Wno-weak-vtables', '-Wno-documentation-unknown-command', '-Wno-old-style-cast', '-Wno-missing-variable-declarations', '-Wno-missing-prototypes', '-Weverything', '-DMVERBOSE=4', '-fsanitize=address', '-DREAD_CHECK']
link_args = ['-lasan']

dl = meson.get_compiler('c').find_library('dl', required : false)
thread = meson.get_compiler('c').find_library('pthread', required : false)

base_files = ['mem.c', 'wintools.c']
hlapi_files = ['hlapi/windll.cpp', 'hlapi/winprocess.cpp', 'hlapi/winprocesslist.cpp']

example = executable(
	'example',
	files(base_files + ['example.cpp', 'vmmem.c'] + hlapi_files),
  c_args : compile_args + ['-DLMODE=MODE_EXTERNAL'],
  cpp_args : compile_args + ['-DLMODE=MODE_EXTERNAL'],
  link_args : compile_args + link_args
)

example_lib = shared_library(
	'example',
	files(base_files + ['example.cpp', 'intmem.c'] + hlapi_files),
  c_args : ['-DLMODE=MODE_QEMU_INJECT', '-DMVERBOSE=4'],
  cpp_args : ['-DLMODE=MODE_QEMU_INJECT', '-DMVERBOSE=4'],
  link_args : compile_args + link_args,
  dependencies: [dl]
)

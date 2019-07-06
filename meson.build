project('vmread', 'cpp', 'c', default_options : ['c_std=c99', 'c++_std=c++11'])

compile_args = ['-pedantic', '-Werror', '-DMVERBOSE=4', '-fsanitize=address', '-DREAD_CHECK']
link_args = ['-lasan']
cpp_compile_args = []
c_compile_args = []

try_args = ['-Weverything', '-Wno-padded', '-Wno-sign-conversion', '-Wno-padded', '-Wno-documentation-unknown-command', '-Wno-missing-variable-declarations']
try_args_c = ['-Wno-missing-prototypes']
try_args_cpp = ['-Wno-padded', '-Wno-c++98-compat', '-Wno-padded', '-Wno-c++98-compat-pedantic', '-Wno-weak-vtables', '-Wno-documentation-unknown-command', '-Wno-old-style-cast']

cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

foreach arg : try_args
  if cc.has_argument(arg)
    compile_args += arg
  elif cxx.has_argument(arg)
    cpp_compile_args += arg
  endif
endforeach

foreach arg : try_args_cpp
  if cc.has_argument(arg)
    cpp_compile_args += arg
  endif
endforeach

foreach arg : try_args_c
  if cc.has_argument(arg)
    c_compile_args += arg
  endif
endforeach

dl = meson.get_compiler('c').find_library('dl', required : false)
thread = meson.get_compiler('c').find_library('pthread', required : false)

base_files = ['mem.c', 'wintools.c', 'pmparser.c']
hlapi_files = ['hlapi/windll.cpp', 'hlapi/winprocess.cpp', 'hlapi/winprocesslist.cpp']

example = executable(
	'example',
	files(base_files + ['example.cpp', 'vmmem.c'] + hlapi_files),
  c_args : c_compile_args + compile_args + ['-DLMODE=MODE_EXTERNAL'],
  cpp_args : cpp_compile_args + compile_args + ['-DLMODE=MODE_EXTERNAL'],
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

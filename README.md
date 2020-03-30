# A library to read/write memory to Windows running inside of KVM

wintools.h and mem.h provide most of the functions callable to interract with the Windows VM, while hlapi abstracts everything in a bit simpler to use manner (requires C++).

Rust bindings are available in a [separate repository](https://github.com/Heep042/vmread-rs).

##### Compiling
Minimum language standard: C99
The current example project is in C++, requiring at least C++11 with template support, but the C version also exists, which works fine on a C99 compiler.

Use meson and ninja to compile the example programs

Use make to compile the kernel module

##### Performance
Internal (QEMU inject) mode is roughly 7 times faster than external mode. However, it is possible to use the kernel module to map the memory space of QEMU into the external process, mitigating the performance penalty. Also, when performing larger reads, the memcpy quickly reaches its peak speed and external mode begins to catch up. Performance numbers are shown below.

![alt text](https://github.com/Heep042/vmread/raw/master/rwperf.png "Read performance")


##### Frequent issues
Make sure to use the Q35 chipset on the KVM guest, unless it is running Windows XP. Otherwise, the library may not work correctly.
Kmod mapping is not guaranteed to work properly or for extended periods of time if the VM is not set up to use hugepages.

##### Licensing note
While most of the codebase is under the MIT license, the kernel module (kmem.c file) is licensed under GNU GPLv2.


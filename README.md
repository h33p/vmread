# A library to read/write memory to Windows running inside of KVM

wintools.h and mem.h provide most of the functions callable to interract with the Windows VM, while hlapi abstracts everything in a bit simpler to use manner (requires C++).

##### Compiling
Minimum language standard: C99
The current example project is in C++, requiring at least C++11 with template support, but the C version also exists, which works fine on a C99 compiler.

Use meson and ninja to compile the example programs

Use make to compile the kernel module

##### Performance
Internal (QEMU inject) mode is roughly 24 times faster than external mode. However, it is possible to use the kernel module to map the memory space of QEMU into the external process, mitigating the performance penalty. Also, when performing larger reads, the memcpy quickly reaches its peak speed and external mode begins to catch up. Performance numbers are shown below.

External (no kmod):
```
Reads of size 0x10000: 3154.57 Mb/s; 16 calls; 50473.19 Calls/s
Reads of size 0x1000: 1007.05 Mb/s; 256 calls; 257804.63 Calls/s
Reads of size 0x100: 102.31 Mb/s; 4096 calls; 419071.00 Calls/s
Reads of size 0x10: 6.32 Mb/s; 65536 calls; 414079.83 Calls/s
Reads of size 0x8: 3.21 Mb/s; 131072 calls; 420928.23 Calls/s
```

Internal / External with kmod:
```
Reads of size 0x10000: 7042.25 Mb/s; 16 calls; 112676.06 Calls/s
Reads of size 0x1000: 7042.25 Mb/s; 256 calls; 1802816.90 Calls/s
Reads of size 0x100: 1663.89 Mb/s; 4096 calls; 6815307.82 Calls/s
Reads of size 0x10: 150.31 Mb/s; 65536 calls; 9850593.72 Calls/s
Reads of size 0x8: 78.96 Mb/s; 131072 calls; 10349968.41 Calls/s
```

Performance difference:
```
0x10000: ~2.23 times
0x1000: ~7 times
0x100: ~16.26 times
0x10: ~23.78 times
0x8 ~24.36 times
```

##### Frequent issues
Make sure to use the Q35 chipset on the KVM guest, unless it is running Windows XP. Otherwise, the library may not work correctly.
Kmod mapping is not guaranteed to work properly or for extended periods of time if the VM is not set up to use hugepages.

##### Licensing note
While most of the codebase is under the MIT license, the kernel module (kmem.c file) is licensed under GNU GPLv2.

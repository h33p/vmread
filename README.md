# A library to read/write memory to Windows running inside of KVM

wintools.h and mem.h provide most of the functions callable to interract with the Windows VM, while hlapi abstracts everything in a bit simpler to use manner (requires C++).

##### Compiling
Minimum language standard: C99
The current example project is in C++, requiring at least C++11 with template support, but the C version also exists, which works fine on a C99 compiler.

##### Performance
Internal (QEMU inject) mode is roughly 32 times faster than external mode. However, when performing reads that are larger than one page in size, the memcpy quickly reaches its peak speed and external mode begins to catch up. Performance numbers are shown below.

External:
```
Reads of size 0x10000: 609.38 Mb/s; 16 calls; 9750.15 Calls/s
Reads of size 0x1000: 297.62 Mb/s; 256 calls; 76190.48 Calls/s
Reads of size 0x100: 30.51 Mb/s; 4096 calls; 124984.74 Calls/s
Reads of size 0x10: 2.19 Mb/s; 65536 calls; 143368.74 Calls/s
Reads of size 0x8: 1.14 Mb/s; 131072 calls; 149837.67 Calls/s
```

Internal:
```
Reads of size 0x10000: 4587.16 Mb/s; 16 calls; 73394.50 Calls/s
Reads of size 0x1000: 4464.29 Mb/s; 256 calls; 1142857.14 Calls/s
Reads of size 0x100: 956.02 Mb/s; 4096 calls; 3915869.98 Calls/s
Reads of size 0x10: 66.76 Mb/s; 65536 calls; 4375191.94 Calls/s
Reads of size 0x8: 36.94 Mb/s; 131072 calls; 4842323.04 Calls/s
```

Performance difference:
```
0x10000: ~7.52 times
0x1000: ~15 times
0x100: ~31.33 times
0x10: ~30.48 times
0x8 ~32.69 times
```

##### Frequent issues
Make sure to use the Q35 chipset on the KVM guest, unless it is running Windows XP. Otherwise, the library may not work correctly.

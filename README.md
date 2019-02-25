# A library to read/write memory to Windows running inside of KVM

wintools.h and mem.h provide most of the functions callable to interract with the Windows VM, while hlapi abstracts everything in a bit simpler to use manner (requires C++).

##### Compiling
Minimum language standard: C99
The current example project is in C++, requiring at least C++11 with template support, but the C version also exists, which works fine on a C99 compiler.

##### Frequent issues
Make sure to use the Q35 chipset on the KVM guest, unless it is running Windows XP. Otherwise, the library may not work correctly.

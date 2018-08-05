#include "mem.h"
#include <string.h>

/* Implementation for direct memory reads, used when injected into the QEMU process */

#define KFIX 0x80000000
#define KFIX2(x) ((x) < KFIX ? (x) : ((x) - KFIX))

int MemRead(ProcessData* data, uint64_t localAddr, uint64_t remoteAddr, size_t len)
{
	memcpy((void*)localAddr, (void*)(KFIX2(remoteAddr) + data->mapsStart), len);
	return len;
}

int MemReadMul(ProcessData* data, RWInfo* rdata, size_t num)
{
	int flen = 0;
	size_t i;
	for (i = 0; i < num; i++) {
		memcpy((void*)rdata[i].local, (void*)(KFIX2(rdata[i].remote) + data->mapsStart), rdata[i].size);
		flen += rdata[i].size;
	}
    return flen;
}

int MemWrite(ProcessData* data, uint64_t localAddr, uint64_t remoteAddr, size_t len)
{
	memcpy((void*)(KFIX2(remoteAddr) + data->mapsStart), (void*)localAddr, len);
    return len;
}

int MemWriteMul(ProcessData* data, RWInfo* wdata, size_t num)
{
	int flen = 0;
	size_t i;
	for (i = 0; i < num; i++) {
		memcpy((void*)(KFIX2(wdata[i].remote) + data->mapsStart), (void*)wdata[i].local, wdata[i].size);
		flen += wdata[i].size;
	}
    return flen;
}

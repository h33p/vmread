#include "wintools.h"
#include "mem.h"
#include "pmparser.h"

static int CheckLow(WinCtx* ctx, uint64_t* pml4, uint64_t* kernelEntry);

int InitializeContext(WinCtx* ctx, pid_t pid, int version)
{
	uint64_t pml4, kernelEntry;

	procmaps_struct* maps = pmparser_parse(pid);

	if (!maps)
		return 0;

	procmaps_struct* tempMaps = NULL;
	procmaps_struct* largestMaps = NULL;

	while ((tempMaps = pmparser_next()) != NULL)
		if (!largestMaps || tempMaps->length > largestMaps->length)
			largestMaps = tempMaps;

	if (!largestMaps)
		return 0;

	ctx->process.pid = pid;
	ctx->process.mapsStart = (uint64_t)largestMaps->addr_start;
	ctx->process.mapsSize = largestMaps->length;

	pmparser_free(maps);

	if (!CheckLow(ctx, &pml4, &kernelEntry))
		return 0;

	return 1;
}

/*
  The low stub (if exists), contains PML4 and KernelEntry point.
  Credits: PCILeech
*/
static int CheckLow(WinCtx* ctx, uint64_t* pml4, uint64_t* kernelEntry)
{
	int i, o;
	char buf[0x10000];
	for (i = 0; i < 10; i++) {
		MemRead(&ctx->process, (uint64_t)buf, i * 0x10000, 0x10000);
		for (o = 0; o < 0x10000; o += 0x1000) {
			if(0x00000001000600E9 ^ (0xffffffffffff00ff & *(uint64_t*)(buf + o)))
				continue;
			if(0xfffff80000000000 ^ (0xfffff80000000000 & *(uint64_t*)(buf + o + 0x70)))
				continue;
			if(0xffffff0000000fff & *(uint64_t*)(buf + o + 0xa0))
				continue;
			*pml4 = *(uint64_t*)(buf + o + 0xa0);
			*kernelEntry = *(uint64_t*)(buf + o + 0x70);
			return 1;
		}
	}
	return 0;
}

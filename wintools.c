#include "wintools.h"
#include "mem.h"
#include "pmparser.h"
#include <errno.h>

static int CheckLow(WinCtx* ctx, uint64_t* pml4, uint64_t* kernelEntry);
static uint64_t FindNTKernel(WinCtx* ctx, uint64_t kernelEntry);

int InitializeContext(WinCtx* ctx, pid_t pid, int version)
{
	uint64_t pml4, kernelEntry;

	procmaps_struct* maps = pmparser_parse(pid);

	if (!maps)
		return 1;

	procmaps_struct* tempMaps = NULL;
	procmaps_struct* largestMaps = NULL;

	while ((tempMaps = pmparser_next()) != NULL)
		if (!largestMaps || tempMaps->length > largestMaps->length)
			largestMaps = tempMaps;

	if (!largestMaps)
		return 2;

	ctx->process.pid = pid;
	ctx->process.mapsStart = (uint64_t)largestMaps->addr_start;
	ctx->process.mapsSize = largestMaps->length;

	pmparser_free(maps);

	if (!CheckLow(ctx, &pml4, &kernelEntry))
		return 3;

	MSG(2, "PML4:\t%lx\t| KernelEntry:\t%lx\n", pml4, kernelEntry);

	ctx->dirBase = pml4;
	ctx->ntKernel = FindNTKernel(ctx, kernelEntry);

	if (!ctx->ntKernel)
		return 4;

	MSG(2, "Kernel Base:\t%lx (%lx)\n", ctx->ntKernel, VTranslate(&ctx->process, ctx->dirBase, ctx->ntKernel));

	return 0;
}

/*
  The low stub (if exists), contains PML4 (kernel DirBase) and KernelEntry point.
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

static uint64_t FindNTKernel(WinCtx* ctx, uint64_t kernelEntry)
{
	uint64_t i, o, p, u;
	char buf[0x10000];

	for (i = kernelEntry & ~0x1fffff; i > kernelEntry - 0x20000000; i -= 0x200000) {
		for (o = 0; o < 0x20; o++) {
			VMemRead(&ctx->process, ctx->dirBase, (uint64_t)buf, i + 0x10000 * o, 0x10000);
			for (p = 0; p < 0x10000; p += 0x1000) {
				if (*(short*)(buf + p) == 0x5a4d) { /* MZ magic */
					int kdbg = 0, poolCode = 0;
					for (u = 0; u < 0x1000; u++) {
						kdbg = kdbg || *(uint64_t*)(buf + p + u) == 0x4742444b54494e49;
						poolCode = poolCode || *(uint64_t*)(buf + p + u) == 0x45444f434c4f4f50;
						if (kdbg & poolCode)
							return i + 0x10000 * o + p;
					}
				}
			}
		}
	}

	return 0;
}

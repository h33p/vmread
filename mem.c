#include "mem.h"
#include <string.h>
#include <stdio.h>

#ifndef NO_ASSERTS
#include <assert.h>
#endif

const uint64_t PMASK = (~0xfull << 8) & 0xfffffffffull;
static void FillRWInfo(ProcessData* data, uint64_t dirBase, RWInfo* info, int* count, uint64_t local, uint64_t remote, size_t len);
static int FillRWInfoMul(ProcessData* data, uint64_t dirBase, RWInfo* origData, RWInfo* info, size_t count);
static int CalculateDataCount(RWInfo* info, size_t count);

int VMemRead(ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size)
{
	if ((remote >> 12ull) == ((remote + size) >> 12ull))
		return MemRead(data, local, VTranslate(data, dirBase, remote), size);

	int dataCount = ((size - 1) / 0x1000) + 2;
	RWInfo rdata[dataCount];
	FillRWInfo(data, dirBase, rdata, &dataCount, local, remote, size);
	return MemReadMul(data, rdata, dataCount);
}

int VMemWrite(ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size)
{
	if ((remote >> 12ull) == ((remote + size) >> 12ull))
		return MemWrite(data, local, VTranslate(data, dirBase, remote), size);

	int dataCount = ((size - 1) / 0x1000) + 2;
	RWInfo wdata[dataCount];
	FillRWInfo(data, dirBase, wdata, &dataCount, local, remote, size);
	return MemWriteMul(data, wdata, dataCount);
}

uint64_t VMemReadU64(ProcessData* data, uint64_t dirBase, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, VTranslate(data, dirBase, remote), sizeof(uint64_t));
	return dest;
}

uint64_t VMemWriteU64(ProcessData* data, uint64_t dirBase, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, VTranslate(data, dirBase, remote), sizeof(uint64_t));
	return dest;
}

uint64_t MemReadU64(ProcessData* data, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, remote, sizeof(uint64_t));
	return dest;
}

uint64_t MemWriteU64(ProcessData* data, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, remote, sizeof(uint64_t));
	return dest;
}

int VMemReadMul(ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num)
{
	int dataCount = CalculateDataCount(info, num);
	RWInfo readInfo[dataCount];
	dataCount = FillRWInfoMul(data, dirBase, info, readInfo, num);
	return MemReadMul(data, readInfo, dataCount);
}

int VMemWriteMul(ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num)
{
	int dataCount = CalculateDataCount(info, num);
	RWInfo writeInfo[dataCount];
	dataCount = FillRWInfoMul(data, dirBase, info, writeInfo, num);
	return MemWriteMul(data, writeInfo, dataCount);
}

/*
  Translates a virtual address to a physical one. This (most likely) is windows specific and might need extra work to work on Linux target.
*/

uint64_t VTranslate(ProcessData* data, uint64_t dirBase, uint64_t address)
{
	dirBase &= ~0xf;

	uint64_t pageOffset = address & ~(~0ul << PAGE_OFFSET_SIZE);
	uint64_t pte = ((address >> 12) & (0x1ffll));
	uint64_t pt = ((address >> 21) & (0x1ffll));
	uint64_t pd = ((address >> 30) & (0x1ffll));
	uint64_t pdp = ((address >> 39) & (0x1ffll));

	uint64_t pdpe = MemReadU64(data, dirBase + 8 * pdp);
	if (~pdpe & 1)
		return 0;

	uint64_t pde = MemReadU64(data, (pdpe & PMASK) + 8 * pd);
	if (~pde & 1)
		return 0;

	/* 1GB large page, use pde's 12-34 bits */
	if (pde & 0x80)
		return (pde & (~0ull << 42 >> 12)) + (address & ~(~0ull << 30));

	uint64_t pteAddr = MemReadU64(data, (pde & PMASK) + 8 * pt);
	if (~pteAddr & 1)
		return 0;

	/* 2MB large page */
	if (pteAddr & 0x80)
		return (pteAddr & PMASK) + (address & ~(~0ull << 21));

	address = MemReadU64(data, (pteAddr & PMASK) + 8 * pte) & PMASK;

	if (!address)
		return 0;

	return address + pageOffset;
}

/* Static functions */

static int CalculateDataCount(RWInfo* info, size_t count)
{
	int ret = 0;

	for (size_t i = 0; i < count; i++)
		ret += ((info[i].remote + info[i].size - 1) >> 12ull) - (info[i].remote >> 12ull);

	return ret;
}

static int FillRWInfoMul(ProcessData* data, uint64_t dirBase, RWInfo* origData, RWInfo* info, size_t count)
{
	int ret = 0;
	for (size_t i = 0; i < count; i++) {
		int count = 0;
		FillRWInfo(data, dirBase, info + ret, &count, origData[i].local, origData[i].remote, origData[i].size);
		ret += count;
	}
	return ret;
}

static void FillRWInfo(ProcessData* data, uint64_t dirBase, RWInfo* info, int* count, uint64_t local, uint64_t remote, size_t len)
{
	memset(info, 0, sizeof(RWInfo) * *count);
	info[0].local = local;
	info[0].remote = VTranslate(data, dirBase, remote);
	info[0].size = 0x1000 - (remote & 0xfff);

	uint64_t curSize = info[0].size;

#ifndef NO_ASSERTS
	assert(!((remote + curSize) & 0xfff));
#endif

	uint64_t tlen = 0;

	int i = 1;
	for (; curSize < len; curSize += 0x1000) {
		info[i].local = local + curSize;
		info[i].remote = VTranslate(data, dirBase, remote + curSize);
		info[i].size = len - curSize;
		if (info[i].size > 0x1000)
		    info[i].size = 0x1000;
		if (info[i].remote) {
			tlen += info[i].size;
			i++;
		}

		if (tlen > len)
			printf("LEN EXCEEDED\n");
	}

	if (tlen > len)
		printf("LEN EXCEEDED\n");

	*count = i;
}

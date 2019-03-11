#include "mem.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef NO_ASSERTS
#include <assert.h>
#endif

static const uint64_t PMASK = (~0xfull << 8) & 0xfffffffffull;
static void FillRWInfo(const ProcessData* data, uint64_t dirBase, RWInfo* info, int* count, uint64_t local, uint64_t remote, size_t len);
static int FillRWInfoMul(const ProcessData* data, uint64_t dirBase, RWInfo* origData, RWInfo* info, size_t count);
static int CalculateDataCount(RWInfo* info, size_t count);

ssize_t VMemRead(const ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size)
{
	if ((remote >> 12ull) == ((remote + size) >> 12ull))
		return MemRead(data, local, VTranslate(data, dirBase, remote), size);

	int dataCount = (int)((size - 1) / 0x1000) + 2;
	RWInfo rdataStack[MAX_BATCHED_RW];
	RWInfo* rdata = rdataStack;

	if (dataCount > MAX_BATCHED_RW)
		rdata = (RWInfo*)malloc(sizeof(RWInfo) * dataCount);

	FillRWInfo(data, dirBase, rdata, &dataCount, local, remote, size);
	ssize_t ret = MemReadMul(data, rdata, dataCount);

	if (dataCount > MAX_BATCHED_RW)
		free(rdata);

	return ret;
}

ssize_t VMemWrite(const ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size)
{
	if ((remote >> 12ull) == ((remote + size) >> 12ull))
		return MemWrite(data, local, VTranslate(data, dirBase, remote), size);

	int dataCount = (int)((size - 1) / 0x1000) + 2;
	RWInfo wdataStack[MAX_BATCHED_RW];
	RWInfo* wdata = wdataStack;

	if (dataCount > MAX_BATCHED_RW)
		wdata = (RWInfo*)malloc(sizeof(RWInfo) * dataCount);

	FillRWInfo(data, dirBase, wdata, &dataCount, local, remote, size);
	ssize_t ret = MemWriteMul(data, wdata, dataCount);

	if (dataCount > MAX_BATCHED_RW)
		free(wdata);

	return ret;
}

uint64_t VMemReadU64(const ProcessData* data, uint64_t dirBase, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, VTranslate(data, dirBase, remote), sizeof(uint64_t));
	return dest;
}

uint64_t VMemWriteU64(const ProcessData* data, uint64_t dirBase, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, VTranslate(data, dirBase, remote), sizeof(uint64_t));
	return dest;
}

uint64_t MemReadU64(const ProcessData* data, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, remote, sizeof(uint64_t));
	return dest;
}

uint64_t MemWriteU64(const ProcessData* data, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, remote, sizeof(uint64_t));
	return dest;
}

ssize_t VMemReadMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num)
{
	int dataCount = CalculateDataCount(info, num);
	RWInfo readInfoStack[MAX_BATCHED_RW];
	RWInfo* readInfo = readInfoStack;

	if (num > MAX_BATCHED_RW)
		readInfo = (RWInfo*)malloc(sizeof(RWInfo) * num);

	dataCount = FillRWInfoMul(data, dirBase, info, readInfo, num);
	ssize_t ret = MemReadMul(data, readInfo, dataCount);

	if (num > MAX_BATCHED_RW)
		free(readInfo);

	return ret;
}

ssize_t VMemWriteMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num)
{
	int dataCount = CalculateDataCount(info, num);
	RWInfo writeInfoStack[MAX_BATCHED_RW];
	RWInfo* writeInfo = writeInfoStack;

	if (num > MAX_BATCHED_RW)
			writeInfo = (RWInfo*)malloc(sizeof(RWInfo) * num);

	dataCount = FillRWInfoMul(data, dirBase, info, writeInfo, num);
	ssize_t ret = MemWriteMul(data, writeInfo, dataCount);

	if (num > MAX_BATCHED_RW)
		free(writeInfo);

	return ret;
}

/*
  Translates a virtual address to a physical one. This (most likely) is windows specific and might need extra work to work on Linux target.
*/

uint64_t VTranslate(const ProcessData* data, uint64_t dirBase, uint64_t address)
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

static int FillRWInfoMul(const ProcessData* data, uint64_t dirBase, RWInfo* origData, RWInfo* info, size_t count)
{
	int ret = 0;
	for (size_t i = 0; i < count; i++) {
		int lcount = 0;
		FillRWInfo(data, dirBase, info + ret, &lcount, origData[i].local, origData[i].remote, origData[i].size);
		ret += lcount;
	}
	return ret;
}

static void FillRWInfo(const ProcessData* data, uint64_t dirBase, RWInfo* info, int* count, uint64_t local, uint64_t remote, size_t len)
{
	memset(info, 0, sizeof(RWInfo) * *count);
	info[0].local = local;
	info[0].remote = VTranslate(data, dirBase, remote);
	info[0].size = 0x1000 - (remote & 0xfff);

#ifndef NO_ASSERTS
	assert(!((remote + info[0].size) & 0xfff));
#endif

	if (info[0].size > len)
		info[0].size = len;

	uint64_t curSize = info[0].size;

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
	}

	*count = i;
}

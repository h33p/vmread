#include "mem.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef NO_ASSERTS
#include <assert.h>
#endif

#if (LMODE() == MODE_EXTERNAL() && !defined(KMOD_MEMMAP))
#define USE_PAGECACHE
#endif

/* For how long should the cached page be valid */
#ifndef VT_CACHE_TIME_MS
#define VT_CACHE_TIME_MS 1
#endif

static size_t vtCacheTimeMS = VT_CACHE_TIME_MS;

#define VT_CACHE_TIME_NS vtCacheTimeMS * 1000000ll

/*
  This is used to cache the pages touched last bu reads of VTranslate, this increases the performance of external mode by at least 2x for multiple consequitive reads in common area. Cached page expires after a set interval which should be small enough not to cause very serious harm
*/

// This makes the cache fit in 12 pages
#ifndef TLB_SIZE
#define TLB_SIZE 1024
#endif

typedef struct {
	uint64_t page;
	uint64_t dirBase;
	uint64_t translation;
} tlbentry_t;

typedef struct {
	size_t tlbHits;
	size_t tlbMisses;

	struct timespec curTime;
	struct timespec entryTimes[TLB_SIZE];
	tlbentry_t entries[TLB_SIZE];

#ifdef USE_PAGECACHE
	uint64_t pageCachePage[4];
	char pageCache[4][0x1000];
	struct timespec pageCacheTime[4];
#endif
} _tlb_t;

static __thread _tlb_t vtTlb = {
#ifdef USE_PAGECACHE
	.pageCachePage = {0, 0, 0, 0},
#endif
	.tlbHits = 0,
	.tlbMisses = 0
};

static const uint64_t PMASK = (~0xfull << 8) & 0xfffffffffull;

static size_t GetTlbIndex(uint64_t address);
static struct timespec GetTime(void);
static void VtUpdateCurTime(_tlb_t* tlb);
static uint64_t VtMemReadU64(const ProcessData* data, _tlb_t* tlb, size_t idx, uint64_t address);
static uint64_t VtCheckCachedResult(_tlb_t* tlb, uint64_t inAddress, uint64_t dirBase);
static void VtUpdateCachedResult(_tlb_t* tlb, uint64_t inAddress, uint64_t address, uint64_t dirBase);
static uint64_t VTranslateInternal(const ProcessData* data, _tlb_t* tlb, uint64_t dirBase, uint64_t address);

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

	if (rdata != rdataStack)
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

	if (wdata != wdataStack)
		free(wdata);

	return ret;
}

uint64_t VMemReadU64(const ProcessData* data, uint64_t dirBase, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, VTranslate(data, dirBase, remote), sizeof(uint64_t));
	return dest;
}

ssize_t VMemWriteU64(const ProcessData* data, uint64_t dirBase, uint64_t remote, uint64_t value)
{
	return MemRead(data, (uint64_t)&value, VTranslate(data, dirBase, remote), sizeof(uint64_t));
}

uint64_t MemReadU64(const ProcessData* data, uint64_t remote)
{
	uint64_t dest;
	MemRead(data, (uint64_t)&dest, remote, sizeof(uint64_t));
	return dest;
}

ssize_t MemWriteU64(const ProcessData* data, uint64_t remote, uint64_t value)
{
	return MemRead(data, (uint64_t)&value, remote, sizeof(uint64_t));
}

ssize_t VMemReadMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num)
{
	int dataCount = CalculateDataCount(info, num);
	RWInfo readInfoStack[MAX_BATCHED_RW];
	RWInfo* readInfo = readInfoStack;

	if (dataCount > MAX_BATCHED_RW)
		readInfo = (RWInfo*)malloc(sizeof(RWInfo) * dataCount);

	dataCount = FillRWInfoMul(data, dirBase, info, readInfo, num);
	ssize_t ret = MemReadMul(data, readInfo, dataCount);

	if (readInfo != readInfoStack)
		free(readInfo);

	return ret;
}

ssize_t VMemWriteMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num)
{
	int dataCount = CalculateDataCount(info, num);
	RWInfo writeInfoStack[MAX_BATCHED_RW];
	RWInfo* writeInfo = writeInfoStack;

	if (dataCount > MAX_BATCHED_RW)
			writeInfo = (RWInfo*)malloc(sizeof(RWInfo) * dataCount);

	dataCount = FillRWInfoMul(data, dirBase, info, writeInfo, num);
	ssize_t ret = MemWriteMul(data, writeInfo, dataCount);

	if (writeInfo != writeInfoStack)
		free(writeInfo);

	return ret;
}

uint64_t VTranslate(const ProcessData* data, uint64_t dirBase, uint64_t address)
{
	dirBase &= ~0xf;

	_tlb_t* tlb = &vtTlb;

	uint64_t cachedVal = VtCheckCachedResult(tlb, address, dirBase);

	if (cachedVal)
		return cachedVal;

	cachedVal = VTranslateInternal(data, tlb, dirBase, address);

	VtUpdateCachedResult(tlb, address, cachedVal, dirBase);

	return cachedVal;
}

void SetMemCacheTime(size_t newTime)
{
	vtCacheTimeMS = newTime;
}

size_t GetDefaultMemCacheTime(void)
{
	return VT_CACHE_TIME_MS;
}

tlb_t* GetTlb(void)
{
	return (tlb_t*)&vtTlb;
}

void VerifyTlb(const ProcessData* data, tlb_t* tlbIn, size_t splitCount, size_t splitID)
{
	splitID = splitID % splitCount;

	_tlb_t* tlb = (_tlb_t*)tlbIn;

	size_t start = TLB_SIZE * splitID / splitCount;
	size_t end = TLB_SIZE * (splitID + 1) / splitCount;

	for (size_t i = start; i < end; i++) {
		tlbentry_t entry = tlb->entries[i];
		tlb->entries[i] = (tlbentry_t) {
			.page = entry.page,
			.dirBase = entry.dirBase,
			.translation = VTranslateInternal(data, tlb, entry.page, entry.dirBase)
		};
	}

	struct timespec time = GetTime();

	for (size_t i = start; i < end; i++)
		tlb->entryTimes[i] = time;
}

void FlushTlb(tlb_t* tlb)
{
	struct timespec time = {
		.tv_nsec = 0,
		.tv_sec = 0
	};

	for (size_t i = 0; i < TLB_SIZE; i++)
		((_tlb_t*)tlb)->entryTimes[i] = time;
}

/* Static functions */

static size_t GetTlbIndex(uint64_t address)
{
	return (address >> 12l) % TLB_SIZE;
}

static struct timespec GetTime(void)
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC_COARSE, &time);
	return time;
}

static void VtUpdateCurTime(_tlb_t* tlb)
{
	tlb->curTime = GetTime();
}

static uint64_t VtMemReadU64(const ProcessData* data, _tlb_t* tlb, size_t idx, uint64_t address)
{
#ifdef USE_PAGECACHE
	uint64_t page = address & ~0xfff;

	uint64_t timeDiff = (tlb->curTime.tv_sec - tlb->pageCacheTime[idx].tv_sec) * (uint64_t)1e9 + (tlb->curTime.tv_nsec - tlb->pageCacheTime[idx].tv_nsec);

	if (tlb->pageCachePage[idx] != page || timeDiff >= VT_CACHE_TIME_NS) {
		MemRead(data, (uint64_t)tlb->pageCache[idx], page, 0x1000);
		tlb->pageCachePage[idx] = page;
		tlb->pageCacheTime[idx] = tlb->curTime;
	}

	return *(uint64_t*)(void*)(tlb->pageCache[idx] + (address & 0xfff));
#else
	(void)tlb;
	(void)idx;
	return MemReadU64(data, address);
#endif
}

static uint64_t VtCheckCachedResult(_tlb_t* tlb, uint64_t inAddress, uint64_t dirBase)
{
	VtUpdateCurTime(tlb);
	size_t index = GetTlbIndex(inAddress);
	tlbentry_t tlbEntry = tlb->entries[index];
	struct timespec tlbEntryTime = tlb->entryTimes[index];
	if ((tlbEntry.dirBase == dirBase) && (tlbEntry.page == (inAddress & ~0xfff))) {
		uint64_t timeDiff = (tlb->curTime.tv_sec - tlbEntryTime.tv_sec) * (uint64_t)1e9 + (tlb->curTime.tv_nsec - tlbEntryTime.tv_nsec);

		if (timeDiff < VT_CACHE_TIME_NS) {
			tlb->tlbHits++;
			return tlbEntry.translation | (inAddress & 0xfff);
		}
	}
	return 0;
}

static void VtUpdateCachedResult(_tlb_t* tlb, uint64_t inAddress, uint64_t address, uint64_t dirBase)
{
	size_t index = GetTlbIndex(inAddress);
	tlb->entryTimes[index] = tlb->curTime;
	tlb->entries[index] = (tlbentry_t) {
		.translation = address & ~0xfff,
		.page = inAddress & ~0xfff,
		.dirBase = dirBase
	};
	tlb->tlbMisses++;
}

static uint64_t VTranslateInternal(const ProcessData* data, _tlb_t* tlb, uint64_t dirBase, uint64_t address)
{
	uint64_t pageOffset = address & ~(~0ul << PAGE_OFFSET_SIZE);
	uint64_t pte = ((address >> 12) & (0x1ffll));
	uint64_t pt = ((address >> 21) & (0x1ffll));
	uint64_t pd = ((address >> 30) & (0x1ffll));
	uint64_t pdp = ((address >> 39) & (0x1ffll));

	uint64_t pdpe = VtMemReadU64(data, tlb, 0, dirBase + 8 * pdp);
	if (~pdpe & 1)
		return 0;

	uint64_t pde = VtMemReadU64(data, tlb, 1, (pdpe & PMASK) + 8 * pd);
	if (~pde & 1)
		return 0;

	/* 1GB large page, use pde's 12-34 bits */
	if (pde & 0x80)
		return (pde & (~0ull << 42 >> 12)) + (address & ~(~0ull << 30));

	uint64_t pteAddr = VtMemReadU64(data, tlb, 2, (pde & PMASK) + 8 * pt);
	if (~pteAddr & 1)
		return 0;

	/* 2MB large page */
	if (pteAddr & 0x80)
		return (pteAddr & PMASK) + (address & ~(~0ull << 21));

	address = VtMemReadU64(data, tlb, 3, (pteAddr & PMASK) + 8 * pte) & PMASK;

	if (!address)
		return 0;

	return address + pageOffset;
}


static int CalculateDataCount(RWInfo* info, size_t count)
{
	int ret = 0;

	for (size_t i = 0; i < count; i++)
		ret += 1 + ((info[i].remote + info[i].size - 1) >> 12ull) - (info[i].remote >> 12ull);

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

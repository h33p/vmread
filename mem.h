#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "definitions.h"

#define PAGE_OFFSET_SIZE 12

#define MAX_BATCHED_RW 1024

typedef struct RWInfo
{
	uint64_t local;
	uint64_t remote;
	size_t size;
} RWInfo;

ssize_t MemRead(const ProcessData* data, uint64_t local, uint64_t remote, size_t size);
ssize_t MemWrite(const ProcessData* data, uint64_t local, uint64_t remote, size_t size);
ssize_t MemReadMul(const ProcessData* data, RWInfo* info, size_t num);
ssize_t MemWriteMul(const ProcessData* data, RWInfo* info, size_t num);
uint64_t VMemReadU64(const ProcessData* data, uint64_t dirBase, uint64_t remote);
uint64_t VMemWriteU64(const ProcessData* data, uint64_t dirBase, uint64_t remote);
uint64_t MemReadU64(const ProcessData* data, uint64_t remote);
uint64_t MemWriteU64(const ProcessData* data, uint64_t remote);
ssize_t VMemRead(const ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size);
ssize_t VMemWrite(const ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size);
ssize_t VMemReadMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num);
ssize_t VMemWriteMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num);
uint64_t VTranslate(const ProcessData* data, uint64_t dirBase, uint64_t address);

#ifdef __cplusplus
}
#endif

#endif

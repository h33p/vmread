#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"

#define PAGE_OFFSET_SIZE 12

typedef struct RWInfo
{
	uint64_t local;
	uint64_t remote;
	size_t size;
} RWInfo;

int MemRead(ProcessData* data, uint64_t local, uint64_t remote, size_t size);
int MemWrite(ProcessData* data, uint64_t local, uint64_t remote, size_t size);
int MemReadMul(ProcessData* data, RWInfo* info, size_t num);
int MemWriteMul(ProcessData* data, RWInfo* info, size_t num);
uint64_t MemReadU64(ProcessData* data, uint64_t remote);
uint64_t MemWriteU64(ProcessData* data, uint64_t remote);
int VMemRead(ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size);
int VMemWrite(ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size);
uint64_t VTranslate(ProcessData* data, uint64_t dirBase, uint64_t address);

#ifdef __cplusplus
}
#endif

#endif

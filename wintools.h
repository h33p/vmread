#ifndef WINTOOLS_H
#define WINTOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
  Provides all utilities and structures for Windows operating systems.
*/

#include "definitions.h"

typedef struct WinOffsets
{
	int64_t apl;
	int64_t session;
	int64_t image_file_name;
	int64_t dir_base;
	int64_t peb;
	int64_t peb32;
	int64_t thread_list_head;
	int64_t thread_list_entry;
	int64_t teb;
} WinOffsets;

typedef struct WinCtx
{
	ProcessData process;
	WinOffsets offsets;

	uint64_t ntKernel;
	uint64_t initialProcess;
	uint64_t dirBase;
} WinCtx;

int InitializeContext(WinCtx* ctx, pid_t pid, int version);
uint64_t TranslateAddress(WinCtx* ctx, uint64_t addr);

#ifdef __cplusplus
}
#endif

#endif

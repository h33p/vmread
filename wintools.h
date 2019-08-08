#ifndef WINTOOLS_H
#define WINTOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
  Provides all utilities and structures for Windows operating systems.
*/

#include "definitions.h"
#include "winstructs.h"

typedef struct WinOffsets
{
	int64_t apl;
	int64_t session;
	int64_t imageFileName;
	int64_t dirBase;
	int64_t peb;
	int64_t peb32;
	int64_t threadListHead;
	int64_t threadListEntry;
	int64_t teb;
} WinOffsets;

typedef struct WinProc
{
	uint64_t process;
	uint64_t physProcess;
	uint64_t dirBase;
	uint64_t pid;
	char name[16];
} WinProc;

typedef struct WinProcList
{
	WinProc* list;
	size_t size;
} WinProcList;

typedef struct WinExport
{
	char* name;
	uint64_t address;
} WinExport;

typedef struct WinExportList
{
	WinExport* list;
	size_t size;
} WinExportList;

typedef struct WinModule
{
	uint64_t baseAddress;
	uint64_t entryPoint;
	uint64_t sizeOfModule;
	char* name;
	short loadCount;
} WinModule;

typedef struct WinModuleList
{
	WinModule* list;
	size_t size;
} WinModuleList;

typedef struct WinCtx
{
	ProcessData process;
	WinOffsets offsets;
	uint64_t ntKernel;
	uint16_t ntVersion;
	uint32_t ntBuild;
	WinExportList ntExports;
	WinProc initialProcess;
} WinCtx;

int InitializeContext(WinCtx* ctx, pid_t pid);
int FreeContext(WinCtx* ctx);
IMAGE_NT_HEADERS* GetNTHeader(const WinCtx* ctx, const WinProc* process, uint64_t address, uint8_t* header, uint8_t* is64Bit);
int ParseExportTable(const WinCtx* ctx, const WinProc* process, uint64_t moduleBase, IMAGE_DATA_DIRECTORY* exports, WinExportList* outList);
int GenerateExportList(const WinCtx* ctx, const WinProc* process, uint64_t moduleBase, WinExportList* outList);
void FreeExportList(WinExportList list);
uint64_t GetProcAddress(const WinCtx* ctx, const WinProc* process, uint64_t module, const char* procName);
uint64_t FindProcAddress(const WinExportList exports, const char* procName);
WinProcList GenerateProcessList(const WinCtx* ctx);
WinModuleList GenerateModuleList(const WinCtx* ctx, const WinProc* process);
WinModuleList GenerateKernelModuleList(const WinCtx* ctx);
void FreeModuleList(WinModuleList list);
const WinModule* GetModuleInfo(const WinModuleList list, const char* moduleName);
PEB GetPeb(const WinCtx* ctx, const WinProc* process);
PEB32 GetPeb32(const WinCtx* ctx, const WinProc* process);

#ifdef __cplusplus
}
#endif

#endif

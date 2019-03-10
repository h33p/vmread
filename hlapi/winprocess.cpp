#include "hlapi.h"

ModuleIteratableList::iterator ModuleIteratableList::begin()
{
	process->VerifyModuleList();
	return iterator(this);
}

ModuleIteratableList::iterator ModuleIteratableList::end()
{
	process->VerifyModuleList();
	return iterator(this, size);
}

size_t ModuleIteratableList::getSize()
{
	process->VerifyModuleList();
	return size;
}

WriteList::WriteList(const WinProcess* p)
{
	ctx = p->ctx;
	proc = &p->proc;
}

WriteList::~WriteList()
{
	size_t sz = writeList.size();
	for (size_t i = 0; i < sz; i++)
		free((void*)writeList[i].local);
}

void WriteList::Commit()
{
	size_t sz = writeList.size();

	RWInfo* infos = writeList.data();

	uint64_t bufmem = (uint64_t)buffer.data();

	for (size_t i = 0; i < sz; i++)
		infos[i].local += bufmem;

	VMemWriteMul(&ctx->process, proc->dirBase, infos, sz);

	writeList.clear();
	buffer.clear();
}

WinDll* WinProcess::GetModuleInfo(const char* moduleName)
{
	VerifyModuleList();
	for (size_t i = 0; i < modules.size; i++)
		if (!strcmp(moduleName, modules.list[i].info.name))
			return modules.list + i;
	return nullptr;
}

PEB WinProcess::GetPeb()
{
	return ::GetPeb(ctx, &proc);
}

WinProcess::WinProcess()
{
	ctx = nullptr;
	modules.list = nullptr;
	modules.size = 0;
	modules.process = this;
}

WinProcess::WinProcess(const WinProc& p, const WinCtx* c) : WinProcess()
{
	proc = p;
	ctx = c;
}

WinProcess::WinProcess(WinProcess&& rhs)
{
	proc = rhs.proc;
	ctx = rhs.ctx;
	modules.list = rhs.modules.list;
	modules.size = rhs.modules.size;
	modules.process = this;
	rhs.modules.list = nullptr;
	rhs.modules.size = 0;
}

WinProcess::~WinProcess()
{
	if (modules.list)
		for (size_t i = 0; i < modules.size; i++)
			free(modules.list[i].info.name);
	delete[] modules.list;
}

ssize_t WinProcess::Read(uint64_t address, void* buffer, size_t sz)
{
	return VMemRead(&ctx->process, proc.dirBase, (uint64_t)buffer, address, sz);
}

ssize_t WinProcess::Write(uint64_t address, void* buffer, size_t sz)
{
	return VMemWrite(&ctx->process, proc.dirBase, (uint64_t)&buffer, address, sz);
}

void WinProcess::VerifyModuleList()
{
	if (!modules.list) {
		WinModuleList ls = GenerateModuleList(ctx, &proc);
		modules.list = new WinDll[ls.size];
		modules.size = ls.size;
		for (size_t i = 0; i < modules.size; i++)
			modules.list[i] = WinDll(ctx, &proc, ls.list[i]);
		free(ls.list);
	}
}

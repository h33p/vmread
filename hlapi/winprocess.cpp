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

WriteList::WriteList(WinProcess* p)
{
	ctx = p->ctx;
	proc = &p->proc;
}

WriteList::~WriteList()
{
    while (writeList.size()) {
		free((void*)writeList.top().local);
		writeList.pop();
	}
}

void WriteList::Commit()
{
	size_t sz = writeList.size();
	RWInfo* infos = new RWInfo[sz];

    size_t i = 0;
    while (writeList.size()) {
		infos[i++] = writeList.top();
		writeList.pop();
	}

	VMemWriteMul(&ctx->process, proc->dirBase, infos, sz);

	for (i = 0; i < sz; i++)
		free((void*)infos[i].local);

	delete[] infos;
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

WinProcess::WinProcess(WinProc& p, WinCtx* c) : WinProcess()
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

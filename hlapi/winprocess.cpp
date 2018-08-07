#include "hlapi.h"

ModuleIteratableList::iterator ModuleIteratableList::begin()
{
	return iterator(this);
}

ModuleIteratableList::iterator ModuleIteratableList::end()
{
	return iterator(this, size);
}

WinDll* WinProcess::GetModuleInfo(const char* moduleName)
{
	VerifyModuleList();
	for (size_t i = 0; i < modules.size; i++)
		if (!strcmp(moduleName, modules.list[i].info.name))
			return modules.list + i;
	return nullptr;
}

WinProcess::WinProcess()
{
	ctx = nullptr;
	modules.list = nullptr;
	modules.size = 0;
}

WinProcess::WinProcess(WinProc& p, WinCtx* c)
{
	proc = p;
	ctx = c;
	modules.list = nullptr;
	modules.size = 0;
	VerifyModuleList();
}

WinProcess::WinProcess(WinProcess&& rhs)
{
	proc = rhs.proc;
	ctx = rhs.ctx;
	modules.list = rhs.modules.list;
	modules.size = rhs.modules.size;
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

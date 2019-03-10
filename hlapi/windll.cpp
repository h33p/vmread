#include "hlapi.h"

WinExportIteratableList::iterator WinExportIteratableList::begin()
{
	windll->VerifyExportList();
	return iterator(&list);
}

WinExportIteratableList::iterator WinExportIteratableList::end()
{
	windll->VerifyExportList();
	return iterator(&list, list.size);
}

uint64_t WinDll::GetProcAddress(const char* procName)
{
	VerifyExportList();
	return ::FindProcAddress(exports.list, procName);
}

WinDll::WinDll()
{
	ctx = nullptr;
	process = nullptr;
	exports.list.list = nullptr;
	exports.list.size = 0;
	exports.windll = this;
}

WinDll::WinDll(const WinCtx* c, const WinProc* p, WinModule& i)
	: WinDll()
{
	ctx = c;
	process = p;
	info = i;
}

WinDll::WinDll(WinDll&& rhs)
{
	info = rhs.info;
	process = rhs.process;
	ctx = rhs.ctx;
	exports = rhs.exports;
	exports.windll = this;
	rhs.exports.list.list = nullptr;
	rhs.exports.list.size = 0;
}

WinDll::~WinDll()
{
	FreeExportList(exports.list);
}

void WinDll::VerifyExportList()
{
	if (!exports.list.list)
		GenerateExportList(ctx, process, info.baseAddress, &exports.list);
}

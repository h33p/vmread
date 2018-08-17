#ifndef HLAPI_H
#define HLAPI_H

/* A high level C++ wrapper for various memory functions */

#include "../wintools.h"
#include "../mem.h"

#include <stdexcept>
#include <string.h>
#include <vector>

class VMException : public std::exception
{
  public:
	VMException(int status)
	{
		value = status;
	}

	int value;
};

template<typename T>
class WinListIterator
{
  public:
	WinListIterator(T* l)
	{
		list = l;
		count = 0;
	}

	WinListIterator(T* l, size_t c)
	{
		list = l;
		count = c;
	}

	auto& operator*()
	{
		return list->list[count];
	}

	WinListIterator& operator++(int c)
	{
		count += c;
		return *this;
	}

	WinListIterator& operator++()
	{
		return operator++(1);
	}
	WinListIterator& operator--(int c)
	{
		count -= c;
		return *this;
	}

	WinListIterator& operator--()
	{
		return operator--(1);
	}

	bool operator==(WinListIterator& rhs)
	{
		return count == rhs.count && list == rhs.list;
	}

	bool operator!=(WinListIterator& rhs)
	{
		return !operator==(rhs);
	}

  protected:
	size_t count;
  private:
	T* list;
};

class WinExportIteratableList
{
  public:
	using iterator = WinListIterator<WinExportList>;
	iterator begin();
	iterator end();
  private:
	friend iterator;
	friend class WinDll;
	class WinDll* windll;

	WinExportList list;
};

class WinDll
{
  public:
	uint64_t GetProcAddress(const char* procName);
	WinDll();
	WinDll(WinCtx* c, WinProc* p, WinModule& i);
	WinDll(WinDll&& rhs);
	WinDll(WinDll& rhs) = delete;
	~WinDll();

	auto& operator=(WinDll rhs)
	{
		info = rhs.info;
		std::swap(exports.list, rhs.exports.list);
		ctx = rhs.ctx;
		process = rhs.process;
		return *this;
	}

	WinModule info;
	WinExportIteratableList exports;
  private:
	friend WinExportIteratableList;
	WinCtx* ctx;
	WinProc* process;
	void VerifyExportList();
};

class ModuleIteratableList
{
  public:
	using iterator = WinListIterator<ModuleIteratableList>;
	iterator begin();
	iterator end();
	size_t getSize();
  private:
	friend iterator;
	friend class WinProcess;
	class WinProcess* process;
	WinDll* list;
	size_t size;
};

class WriteList
{
  public:
	WriteList(WinProcess*);
	~WriteList();
	void Commit();

	template<typename T>
	void Write(uint64_t address, T& value)
	{
		T* copy = (T*)malloc(sizeof(T));
		*copy = value;
		writeList.push_back({(uint64_t)copy, address, sizeof(T)});
	}

  private:
	std::vector<RWInfo> writeList;
	WinCtx* ctx;
	WinProc* proc;
};

class WinProcess
{
  public:
	WinDll* GetModuleInfo(const char* moduleName);
	PEB GetPeb();
	WinProcess();
	WinProcess(WinProc& p, WinCtx* c);
	WinProcess(WinProcess&& rhs);
	WinProcess(WinProcess& rhs) = delete;
	~WinProcess();

	template<typename T>
	T Read(uint64_t address)
	{
		T ret;
		VMemRead(&ctx->process, proc.dirBase, (uint64_t)&ret, address, sizeof(T));
		return ret;
	}

	template<typename T>
	void Write(uint64_t address, const T& value)
	{
		VMemWrite(&ctx->process, proc.dirBase, (uint64_t)&value, address, sizeof(T));
	}

	auto& operator=(WinProcess rhs)
	{
		std::swap(modules.list, rhs.modules.list);
		std::swap(modules.size, rhs.modules.size);
		ctx = rhs.ctx;
		proc = rhs.proc;
		return *this;
	}

	WinProc proc;
	ModuleIteratableList modules;
  protected:
	friend ModuleIteratableList;
	friend WriteList;
	WinCtx* ctx;
	void VerifyModuleList();
};

class WinProcessList
{
  public:
	using iterator = WinListIterator<WinProcessList>;
	void Refresh();
	WinProcess* FindProc(const char* name);
	iterator begin();
	iterator end();
	WinProcessList();
	WinProcessList(WinCtx* pctx);
	WinProcessList(WinProcessList&& rhs);
	WinProcessList(WinProcessList& rhs) = delete;
	~WinProcessList();

	auto& operator=(WinProcessList rhs)
	{
		std::swap(plist, rhs.plist);
		std::swap(list, rhs.list);
		ctx = rhs.ctx;
		return *this;
	}

  protected:
	friend iterator;
	WinProcList plist;
	WinCtx* ctx;
	WinProcess* list;
	void FreeProcessList();
};

class WinContext
{
  public:

	template<typename T>
	T Read(uint64_t address)
	{
		T ret;
		MemRead(&ctx.process, (uint64_t)&ret, address, sizeof(T));
		return ret;
	}

	template<typename T>
	void Write(uint64_t address, T& value)
	{
		MemWrite(&ctx.process, (uint64_t)&value, address, sizeof(T));
	}

	WinContext(pid_t pid)
	{
	    int ret = InitializeContext(&ctx, pid);
		if (ret)
			throw VMException(ret);
		processList = WinProcessList(&ctx);
	}

	~WinContext()
	{
		FreeContext(&ctx);
	}

	WinProcessList processList;
	WinCtx ctx;
};

#endif

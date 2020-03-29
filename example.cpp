#include "hlapi/hlapi.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <random>
#include <chrono>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

FILE* dfile;

static unsigned long readbench(const WinProcess& process, size_t start, size_t end, size_t chunkSize, size_t chunkCount, size_t totalSize, size_t *readCount, size_t *totalRead)
{
	end -= chunkSize;
	if (end <= start) {
		chunkSize = (end + chunkSize) - start;
		end = start + 1;
	}

	void** buf = (void**)malloc(chunkCount * sizeof(void*));

	for (size_t i = 0; i < chunkCount; i++)
		buf[i] = malloc(chunkSize);

	std::vector<RWInfo> info;
	info.resize(chunkCount);

	size_t read = 0;
	*readCount = 0;
	*totalRead = 0;

	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_int_distribution<size_t> distr(start, end);
	std::uniform_int_distribution<size_t> distrp(0, 0x2000);

	size_t addr = distr(eng);
	for (size_t i = 0; i < chunkCount; i++) {
		size_t addrp = distrp(eng);
		info[i].local = (uint64_t)buf[i];
		info[i].size = chunkSize;
		info[i].remote = addr + addrp;
	}

	auto beginTime = std::chrono::high_resolution_clock::now();

	while(read < totalSize) {
		size_t addr = distr(eng);
		for (size_t i = 0; i < chunkCount; i++) {
			size_t addrp = distrp(eng);
			info[i].local = (uint64_t)buf[i];
			info[i].size = chunkSize;
			info[i].remote = addr + addrp;
		}
		(*totalRead) += VMemReadMul(&process.ctx->process, process.proc.dirBase, info.data(), chunkCount);
		read += chunkSize * chunkCount;
		(*readCount)++;
	}

	auto endTime = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < chunkCount; i++)
		free(buf[i]);

	free(buf);

	*totalRead = read;

	return std::chrono::duration_cast<std::chrono::microseconds>(endTime - beginTime).count();
}

static const size_t chunkSizes[] =
{
	0x10000,
	0x1000,
	0x100,
	0x10,
	0x8
};

static const size_t chunkCounts[] =
{
	32,
	8,
	1
};

static const size_t readSize = 64;

static void runfullbench(FILE* out, const WinProcess& process, size_t start, size_t end)
{
	size_t readCount;
	size_t totalRead;

	for (const size_t i : chunkSizes) {
		fprintf(out, "0x%lx", i);
		for (const size_t o : chunkCounts) {
			unsigned long time = readbench(process, start, end, i, o, 0x100000 * readSize, &readCount, &totalRead);
			double speed = ((double)(totalRead / 0x100000) * 10e5) / time;
			double callSpeed = ((double)readCount * 10e5) / time;
			fprintf(out, ", %.2lf, %.2lf", speed, callSpeed);
		}
		fprintf(out, "\n");
	}
}

__attribute__((constructor))
static void init()
{
	FILE* out = stdout;
	pid_t pid;
#if (LMODE() == MODE_EXTERNAL())
	FILE* pipe = popen("pidof qemu-system-x86_64", "r");
	fscanf(pipe, "%d", &pid);
	pclose(pipe);
#else
	out = fopen("/tmp/testr.txt", "w");
	pid = getpid();
#endif
	fprintf(out, "Using Mode: %s\n", TOSTRING(LMODE));

	vmread_dfile = out;

	try {
		WinContext ctx(pid);
		ctx.processList.Refresh();

		fprintf(out, "Process List:\nPID\tVIRT\t\t\tPHYS\t\tBASE\t\tNAME\n");
		for (auto& i : ctx.processList)
			fprintf(out, "%.4lx\t%.16lx\t%.9lx\t%.9lx\t%s\n", i.proc.pid, i.proc.process, i.proc.physProcess, i.proc.dirBase, i.proc.name);

		for (auto& i : ctx.processList) {
			if (!strcasecmp("steam.exe", i.proc.name)) {
				fprintf(out, "\nLooping process %lx:\t%s\n", i.proc.pid, i.proc.name);

				PEB peb = i.GetPeb();
				short magic = i.Read<short>(peb.ImageBaseAddress);
				fprintf(out, "\tBase:\t%lx\tMagic:\t%hx (valid: %hhx)\n", peb.ImageBaseAddress, magic, (char)(magic == IMAGE_DOS_SIGNATURE));

				fprintf(out, "\tExports:\n");
				for (auto& o : i.modules) {
					fprintf(out, "\t%.8lx\t%.8lx\t%lx\t%s\n", o.info.baseAddress, o.info.entryPoint, o.info.sizeOfModule, o.info.name);
					if (!strcmp("friendsui.DLL", o.info.name)) {
						for (auto& u : o.exports)
							fprintf(out, "\t\t%lx\t%s\n", u.address, u.name);
					}
				}
			}
		}
	
		WinProcess* processes[] = { ctx.processList.FindProc("System"), ctx.processList.FindProc("Steam.exe"), ctx.processList.FindProc("System") };

		for (auto& proc : processes)
			for (auto& i : ctx.systemModuleList.Get(proc))
				if (!strcasecmp(i.info.name, "win32kbase.sys"))
					fprintf(out, "%s kmod export count: %zu\n", i.info.name, i.exports.getSize());

		WinProcess* steam = ctx.processList.FindProc("steam.exe");

		if (steam) {
			WinDll* mod = steam->modules.GetModuleInfo("friendsui.DLL");
			if (mod) {
				fprintf(out, "Performing memory benchmark...\n");
				runfullbench(out, *steam, mod->info.baseAddress, mod->info.baseAddress + mod->info.sizeOfModule);
			}
		}

	} catch (VMException& e) {
		fprintf(out, "Initialization error: %d\n", e.value);
	}


	fclose(out);
}

int main()
{
	return 0;
}

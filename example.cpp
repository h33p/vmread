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

static unsigned long readbench(const WinProcess& process, size_t start, size_t end, size_t chunkSize, size_t totalSize, size_t *readCount)
{
	end -= chunkSize;
	if (end <= start) {
		chunkSize = (end + chunkSize) - start;
		end = start + 1;
	}

	void* buf = malloc(chunkSize);

	size_t read = 0;
	*readCount = 0;

	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_int_distribution<size_t> distr(start, end);

	auto beginTime = std::chrono::high_resolution_clock::now();

	while(read < totalSize) {
		size_t addr = distr(eng);
		VMemRead(&process.ctx->process, process.proc.dirBase, (uint64_t)buf, addr, chunkSize);
		read += chunkSize;
		(*readCount)++;
	}

	auto endTime = std::chrono::high_resolution_clock::now();

	free(buf);

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

static const size_t readSize = 1;

static void runfullbench(FILE* out, const WinProcess& process, size_t start, size_t end)
{
	size_t readCount;
	for (const size_t i : chunkSizes) {
		unsigned long time = readbench(process, start, end, i, 0x100000 * readSize, &readCount);
		double speed = ((double)readSize * 10e5) / time;
		double callSpeed = ((double)readCount * 10e5) / time;
		fprintf(out, "Reads of size 0x%lx: %.2lf Mb/s; %ld calls; %.2lf Calls/s\n", i, speed, readCount, callSpeed);
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

	dfile = out;

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
						fprintf(out, "Performing memory benchmark...\n");
						runfullbench(out, i, o.info.baseAddress, o.info.baseAddress + o.info.sizeOfModule);
					}
				}
			}
		}
	
		WinProcess* processes[] = { ctx.processList.FindProc("System"), ctx.processList.FindProc("Steam.exe"), ctx.processList.FindProc("System") };

		for (auto& proc : processes)
			for (auto& i : ctx.systemModuleList.Get(proc))
				if (!strcasecmp(i.info.name, "win32kbase.sys"))
					fprintf(out, "%s kmod export count: %zu\n", i.info.name, i.exports.getSize());

	} catch (VMException& e) {
		fprintf(out, "Initialization error: %d\n", e.value);
	}


	fclose(out);
}

int main()
{
	return 0;
}

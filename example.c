#include "wintools.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

FILE* dfile;

__attribute__((constructor))
void init()
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

	WinCtx ctx;
	int ret = InitializeContext(&ctx, pid);
	fprintf(out, "Initialization status: %d\n", ret);

	if (!ret) {
		if (0) {
			fprintf(out, "Kernel Export Table:\n");
			for (size_t i = 0; i < ctx.ntExports.size; i++) {
				char* c = (char*)ctx.ntExports.list[i].name;
				while (*c != '\0')
					putc(*c++, out);
				for (size_t o = (c - ctx.ntExports.list[i].name); o < 64; o++)
					putc(' ', out);
				fprintf(out, "%lx\n", ctx.ntExports.list[i].address);
			}
		}

		WinProcList processList = GenerateProcessList(&ctx);

		fprintf(out, "\nProcess List:\nPID\tVIRT\t\t\tPHYS\t\tBASE\t\tNAME\n");
		for (size_t i = 0; i < processList.size; i++)
			fprintf(out, "%lu\t%.16lx\t%.9lx\t%.9lx\t%s\n", processList.list[i].pid, processList.list[i].process, processList.list[i].physProcess, processList.list[i].dirBase, processList.list[i].name);

		for (size_t i = 0; i < processList.size; i++)
			if (!strcmp(processList.list[i].name, "csgo.exe")) {
				WinModuleList list = GenerateModuleList(&ctx, processList.list + i);
				fprintf(out, "Module List for %s:\nBASEADDR\tENTRY_POINT\tMOD_SIZE\tLOAD\tNAME\n", processList.list[i].name);
				for (int o = 0; o < list.size; o++)
					fprintf(out, "%.8lx\t%.8lx\t%.8lx\t%hx\t%s\n", list.list[o].baseAddress, list.list[o].entryPoint, list.list[o].sizeOfModule, list.list[o].loadCount, list.list[o].name);

				FreeModuleList(list);

				break;
			}

		FreeProcessList(processList);
	}
	FreeContext(&ctx);
	fclose(out);
}

int main()
{
	return 0;
}

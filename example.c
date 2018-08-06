#include "wintools.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

FILE* dfile;
extern uint64_t KFIXC;
extern uint64_t KFIXO;

__attribute__((constructor))
void init()
{

	FILE* out = stdout;
	pid_t pid;
#if (LMODE == MODE_EXTERNAL)
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
	/* Testing for XP */
	if (ret) {
		FreeContext(&ctx);
		fprintf(out, "ERROR! Status %d, retrying for XP...\n", ret);
		KFIXC = 0x40000000ll * 4;
		KFIXO = 0x40000000;
		ret = InitializeContext(&ctx, pid);
	}
	fprintf(out, "Initialization status: %d\n", ret);

	if (0) {
		fprintf(out, "Kernel Export Table:\n");
		for (size_t i = 0; i < ctx.ntExports.count; i++) {
			char* c = (char*)ctx.ntExports.list[i].name;
			while (*c != '\0')
				putc(*c++, out);
			for (size_t o = (c - ctx.ntExports.list[i].name); o < 64; o++)
				putc(' ', out);
			fprintf(out, "%lx\n", ctx.ntExports.list[i].address);
		}
	}

	WinProcessList processList = GenerateProcessList(&ctx);

	fprintf(out, "\nProcess List:\nPID\tVIRT\t\t\tPHYS\t\tBASE\t\tNAME\n");
	for (size_t i = 0; i < processList.size; i++)
		fprintf(out, "%lu\t%.16lx\t%.9lx\t%.9lx\t%s\n", processList.list[i].pid, processList.list[i].process, processList.list[i].physProcess, processList.list[i].dirBase, processList.list[i].name);

	free(processList.list);
	FreeContext(&ctx);
	fclose(out);
}

int main()
{
	return 0;
}

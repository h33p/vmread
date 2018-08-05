#include "wintools.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

__attribute__((constructor))
void init()
{
	pid_t pid;
	FILE* pipe = popen("pidof qemu-system-x86_64", "r");
	fscanf(pipe, "%d", &pid);
	pclose(pipe);

	FILE* out = fopen("/tmp/testr.txt", "w");

	WinCtx ctx;
	int ret = InitializeContext(&ctx, pid, 100);
	fprintf(out, "Initialization status: %d\n", ret);
	fclose(out);
}

int main()
{
	return 0;
}

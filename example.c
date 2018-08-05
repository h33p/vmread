#include "wintools.h"
#include <stdio.h>

int main()
{
	pid_t pid;
	FILE* pipe = popen("pidof qemu-system-x86_64", "r");
	fscanf(pipe, "%d", &pid);
	pclose(pipe);

	WinCtx ctx;
	int ret = InitializeContext(&ctx, pid, 100);
	printf("Initialization status: %d\n", ret);
	return 0;
}

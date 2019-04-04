#include "hlapi/hlapi.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

FILE* dfile;

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

		fprintf(out, "Process List:\n");
		for (auto& i : ctx.processList)
			fprintf(out, "%.4lx\t%s\n", i.proc.pid, i.proc.name);

		for (auto& i : ctx.processList) {
			if (!strcasecmp("steam.exe", i.proc.name)) {
				fprintf(out, "\nLooping process %lx:\t%s\n", i.proc.pid, i.proc.name);

				PEB peb = i.GetPeb();
				short magic = i.Read<short>(peb.ImageBaseAddress);
				fprintf(out, "\tBase:\t%lx\tMagic:\t%hx (valid: %hhx)\n", peb.ImageBaseAddress, magic, (char)(magic == IMAGE_DOS_SIGNATURE));

				fprintf(out, "\tExports:\n");
				for (auto& o : i.modules) {
					fprintf(out, "\t%.8lx\t%.8lx\t%lx\t%s\n", o.info.baseAddress, o.info.entryPoint, o.info.sizeOfModule, o.info.name);
					if (!strcmp("Steam.exe", o.info.name))
						for (auto& u : o.exports)
							fprintf(out, "\t\t%lx\t%s\n", u.address, u.name);
				}
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

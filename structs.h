#ifndef STRUCTS_H
#define STRUCTS_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct ProcessData
{
	pid_t pid;
	uint64_t mapsStart;
	uint64_t mapsSize;
} ProcessData;

#endif

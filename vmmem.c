#include "mem.h"
#include <stdlib.h>
#include <sys/uio.h>

/* Memory read implementation using linux process_vm_ functions */

/*
  On windows, first 2^31 physical bytes are really allocated for various PCI device mappings and do not point to actual physical RAM (mmapped region in QEMU case).
*/
#define KFIX 0x80000000
#define KFIX2(x) ((x) < KFIX ? (x) : ((x) - KFIX))

ssize_t process_vm_readv(pid_t pid,
						 const struct iovec *local_iov,
						 unsigned long liovcnt,
						 const struct iovec *remote_iov,
						 unsigned long riovcnt,
						 unsigned long flags);

ssize_t process_vm_writev(pid_t pid,
						  const struct iovec *local_iov,
						  unsigned long liovcnt,
						  const struct iovec *remote_iov,
						  unsigned long riovcnt,
						  unsigned long flags);

int MemRead(ProcessData* data, uint64_t localAddr, uint64_t remoteAddr, size_t len)
{
	struct iovec local;
	struct iovec remote;
	local.iov_base = (void*)localAddr;
	local.iov_len = len;
	remote.iov_base = (void*)(data->mapsStart + KFIX2(remoteAddr));
	remote.iov_len = len;
	return process_vm_readv(data->pid, &local, 1, &remote, 1, 0);
}

int MemReadMul(ProcessData* data, RWInfo* rdata, size_t num)
{
	struct iovec local[num];
	struct iovec remote[num];
	size_t i;
	for (i = 0; i < num; i++) {
		local[i].iov_base = (void*)rdata[i].local;
		local[i].iov_len = rdata[i].size;
		remote[i].iov_base = (void*)(data->mapsStart + KFIX2(rdata[i].remote));
		remote[i].iov_len = rdata[i].size;
	}
    return process_vm_readv(data->pid, local, num, remote, num, 0);
}

int MemWrite(ProcessData* data, uint64_t localAddr, uint64_t remoteAddr, size_t len)
{
	struct iovec local;
	struct iovec remote;
	local.iov_base = (void*)localAddr;
	local.iov_len = len;
	remote.iov_base = (void*)(data->mapsStart + KFIX2(remoteAddr));
	remote.iov_len = len;
	return process_vm_writev(data->pid, &local, 1, &remote, 1, 0);
}

int MemWriteMul(ProcessData* data, RWInfo* wdata, size_t num)
{
	struct iovec local[num];
	struct iovec remote[num];
	size_t i;
	for (i = 0; i < num; i++) {
		local[i].iov_base = (void*)wdata[i].local;
		local[i].iov_len = wdata[i].size;
		remote[i].iov_base = (void*)(data->mapsStart + KFIX2(wdata[i].remote));
		remote[i].iov_len = wdata[i].size;
	}
    return process_vm_writev(data->pid, local, num, remote, num, 0);
}

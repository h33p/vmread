#include "mem.h"
#include <stdlib.h>
#include <sys/uio.h>
#include <stdio.h>
#include <unistd.h>

/* Memory read implementation using linux process_vm_ functions */

/*
  On windows, first 2^31 physical bytes are really allocated for various PCI device mappings and do not point to actual physical RAM (mmapped region in QEMU case).
  It also differs on Windows XP. This should be solved by injecting code into the kernel to retrieve the actual physical memory map of the system
*/
uint64_t KFIXC = 0x80000000;
uint64_t KFIXO = 0x80000000;
#define KFIX2(x) ((x) < KFIXC ? (x) : ((x) - KFIXO))

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

static ssize_t iov_max = -1;

ssize_t MemRead(const ProcessData* data, uint64_t localAddr, uint64_t remoteAddr, size_t len)
{
	struct iovec local;
	struct iovec remote;
	local.iov_base = (void*)localAddr;
	local.iov_len = len;
	remote.iov_base = (void*)(data->mapsStart + KFIX2(remoteAddr));
	remote.iov_len = len;
	return process_vm_readv(data->pid, &local, 1, &remote, 1, 0);
}

ssize_t MemReadMul(const ProcessData* data, RWInfo* rdata, size_t num)
{
	struct iovec local[num];
	struct iovec remote[num];
	size_t i = 0;
	size_t startRead = 0;

	ssize_t ret = 0;

	if (iov_max == -1)
		iov_max = sysconf(_SC_IOV_MAX);

	for (i = 0; i < num; i++) {
		local[i].iov_base = (void*)rdata[i].local;
		local[i].iov_len = rdata[i].size;
		remote[i].iov_base = (void*)(data->mapsStart + KFIX2(rdata[i].remote));
		remote[i].iov_len = rdata[i].size;

		if (i - startRead + 1 >= (size_t)iov_max) {
			ret = process_vm_readv(data->pid, local + startRead, iov_max, remote + startRead, iov_max, 0);
			if (ret == -1)
				return ret;
			startRead = i + 1;
		}
	}

	if (i != startRead)
	    ret = process_vm_readv(data->pid, local + startRead, i - startRead, remote + startRead, i - startRead, 0);

	return ret;
}

ssize_t MemWrite(const ProcessData* data, uint64_t localAddr, uint64_t remoteAddr, size_t len)
{
	struct iovec local;
	struct iovec remote;
	local.iov_base = (void*)localAddr;
	local.iov_len = len;
	remote.iov_base = (void*)(data->mapsStart + KFIX2(remoteAddr));
	remote.iov_len = len;
	return process_vm_writev(data->pid, &local, 1, &remote, 1, 0);
}

ssize_t MemWriteMul(const ProcessData* data, RWInfo* wdata, size_t num)
{
	struct iovec local[num];
	struct iovec remote[num];
	size_t i;
	size_t startWrite = 0;

	ssize_t ret = 0;

	if (iov_max == -1)
		iov_max = sysconf(_SC_IOV_MAX);

	for (i = 0; i < num; i++) {
		local[i].iov_base = (void*)wdata[i].local;
		local[i].iov_len = wdata[i].size;
		remote[i].iov_base = (void*)(data->mapsStart + KFIX2(wdata[i].remote));
		remote[i].iov_len = wdata[i].size;

		if (i - startWrite >= (size_t)iov_max) {
			ret = process_vm_writev(data->pid, local + startWrite, iov_max, remote + startWrite, iov_max, 0);
			if (ret == -1)
				return ret;
			startWrite = i + 1;
		}
	}

	if (i != startWrite)
		ret = process_vm_writev(data->pid, local + startWrite, i - startWrite, remote + startWrite, i - startWrite, 0);

	return ret;
}

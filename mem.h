#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file mem.h
 * @brief Defines memory operations
 *
 * Provides all necessary methods to read and write memory in a VM
 */

#include "definitions.h"

#define PAGE_OFFSET_SIZE 12

#define MAX_BATCHED_RW 1024

typedef struct RWInfo
{
	uint64_t local;
	uint64_t remote;
	size_t size;
} RWInfo;

typedef struct {
	size_t tlbHits;
	size_t tlbMisses;
} tlb_t;

/**
 * @brief Read a piece of data in physical VM address space
 *
 * @param data VM process data
 * @param local local data address
 * @param remote remote data address
 * @param size size of data
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t MemRead(const ProcessData* data, uint64_t local, uint64_t remote, size_t size);

/**
 * @brief Write a piece of data in physical VM address space
 *
 * @param data VM process data
 * @param local local data address
 * @param remote remote data address
 * @param size size of data
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t MemWrite(const ProcessData* data, uint64_t local, uint64_t remote, size_t size);

/**
 * @brief Read multiple pieces of data in physical VM address space
 *
 * @param data VM process data
 * @param info list of information for RW operations
 * @param num number of info atoms
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t MemReadMul(const ProcessData* data, RWInfo* info, size_t num);

/**
 * @brief Write multiple pieces of data in physical VM address space
 *
 * @param data VM process data
 * @param info list of information for RW operations
 * @param num number of info atoms
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t MemWriteMul(const ProcessData* data, RWInfo* info, size_t num);

/**
 * @brief Read a unsigned 64-bit integer in virtual VM address space
 *
 * @param data VM process data
 * @param dirBase page table directory base of a process
 * @param remote remote data address
 *
 * @return
 * Read value, undefined on failure
 */
uint64_t VMemReadU64(const ProcessData* data, uint64_t dirBase, uint64_t remote);

/**
 * @brief Write a unsigned 64-bit integer in virtual VM address space
 *
 * @param data VM process data
 * @param dirBase page table directory base of a process
 * @param remote remote data address
 * @param value value to be written
 *
 * @return
 * 8 on success;
 * -1 on failure
 */
ssize_t VMemWriteU64(const ProcessData* data, uint64_t dirBase, uint64_t remote, uint64_t value);

/**
 * @brief Read a unsigned 64-bit integer in physical VM address space
 *
 * @param data VM process data
 * @param remote remote data address
 *
 * @return
 * Read value, undefined on failure
 */
uint64_t MemReadU64(const ProcessData* data, uint64_t remote);

/**
 * @brief Write a unsigned 64-bit integer in physical VM address space
 *
 * @param data VM process data
 * @param remote remote data address
 * @param value value to be written
 *
 * @return
 * 8 on success;
 * -1 on failure
 */
ssize_t MemWriteU64(const ProcessData* data, uint64_t remote, uint64_t value);

/**
 * @brief Read data in virtual VM address space
 *
 * @param data VM process data
 * @param dirBase page table directory base of a process
 * @param local local data address
 * @param remote remote data address
 * @param size size of data
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t VMemRead(const ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size);

/**
 * @brief Write data in virtual addresss space
 *
 * @param data VM process data
 * @param dirBase page table directory base of a process
 * @param local local data address
 * @param remote remote data address
 * @param size size of data
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t VMemWrite(const ProcessData* data, uint64_t dirBase, uint64_t local, uint64_t remote, size_t size);

/**
 * @brief Read multiple pieces of data in virtual VM address space
 *
 * @param data VM process data
 * @param dirBase page table directory base of a process
 * @param info list of information for RW operations
 * @param num number of info atoms
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t VMemReadMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num);

/**
 * @brief Write multiple pieces of data in virtual VM address space
 *
 * @param data VM process data
 * @param dirBase page table directory base of a process
 * @param info list of information for RW operations
 * @param num number of info atoms
 *
 * @return
 * Data moved on success;
 * -1 otherwise
 */
ssize_t VMemWriteMul(const ProcessData* data, uint64_t dirBase, RWInfo* info, size_t num);

/**
 * @brief Translate a virtual VM address into a physical one
 *
 * @param data VM process data
 * @param dirBase page table directory base of a process
 * @param address virtual address to translate
 *
 * @return
 * Translated linear address;
 * 0 otherwise
 */
uint64_t VTranslate(const ProcessData* data, uint64_t dirBase, uint64_t address);

/**
 * @brief Set translation cache validity time in msecs
 *
 * @param newTime new validity length for a cache entry
 *
 * Defines for how long translation caches (TLB and page buffer) should be valid. Higher values lead to higher
 * performance, but could potentially lead to incorrect translation if the page tables update in that period.
 * Especially dangerous if write operations are to be performed.
 */
void SetMemCacheTime(size_t newTime);

/**
 * @brief Get the default cache validity
 *
 * @return
 * Default cache validity time
 */
size_t GetDefaultMemCacheTime(void);

/**
 * @brief Retrieve current thread's TLB
 *
 * Memory TLB utilizes thread local storage to make the code concurrant. However, it might be beneficial to
 * access the TLB structure to verify its entries asynchronously during idle. This allows to access the
 * said TLB to do just that.
 *
 * @return
 * TLB of the running thread
 */
tlb_t* GetTlb(void);

/**
 * @brief Verify the TLB entries
 *
 * @param data VM process data
 * @param tlb TLB structure to verify
 * @param splitCount how many splits there are
 * @param splitID which slice to verify
 *
 * This allows to verify the TLB structure before initializing a round of memory operations. Useful when the
 * same memory addresses are being accessed in a loop with some delay. During the said delay we could verify
 * the TLB structure in (optionally) multithreaded way to make the memory operations fast.
 *
 * splitCount allows us to split the TLB entries to verify to separate threads. Passing 1 to splitCount makes
 * the function verify the entirety of TLB (single-threaded scenario)
 */
void VerifyTlb(const ProcessData* data, tlb_t* tlb, size_t splitCount, size_t splitID);

/**
 * @brief Flush all TLB entries
 *
 * @param tlb TLB structure to flush
 *
 */
void FlushTlb(tlb_t* tlb);

#ifdef __cplusplus
}
#endif

#endif

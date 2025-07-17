/*
 * issue page table query request to the kernel module.
 */

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "page_table_query.h"

#define DEVICE_PATH	  "/dev/page_table_walker"

int a_global_variable;
volatile int sink;

static int lookup_pagemap(unsigned long *pa, unsigned long va);
// TODO the value of query->va
// TODO show the pa of kernel addr
// TODO decode page attributes of each architecuture
static void print_page_table_query(const struct page_table_query *query, const char *desc)
{
	unsigned long pagemap_pa;

	printf("[pid=%u, va=0x%016llx, %s]\n", query->pid, query->va, desc);
	printf("num_levels=%u\n", query->num_levels);
	printf("pgd_table_kla=0x%016llx\n", query->pgd_table_kla);
	printf("pgd_entry_kla=0x%016llx (pa=0x%016llx), pgd_entry=0x%016llx\n", query->pgd_entry_kla,
		query->pgd_entry_kla - query->kla_start, query->pgd_entry);
	printf("p4d_entry_kla=0x%016llx (pa=0x%016llx), p4d_entry=0x%016llx\n", query->p4d_entry_kla,
		query->p4d_entry_kla - query->kla_start, query->p4d_entry);
	printf("pud_entry_kla=0x%016llx (pa=0x%016llx), pud_entry=0x%016llx\n", query->pud_entry_kla,
		query->pud_entry_kla - query->kla_start, query->pud_entry);
	printf("pmd_entry_kla=0x%016llx (pa=0x%016llx), pmd_entry=0x%016llx\n", query->pmd_entry_kla,
		query->pmd_entry_kla - query->kla_start, query->pmd_entry);
	printf("pte_entry_kla=0x%016llx (pa=0x%016llx), pte_entry=0x%016llx\n", query->pte_entry_kla,
		query->pte_entry_kla - query->kla_start, query->pte_entry);
	printf("pa=0x%016llx\n", query->pa);

	/* check against /proc/pid/pagemap */
	(void)lookup_pagemap(&pagemap_pa, query->va);
	printf("\n");
}

static int query_va(void *va, const char *desc)
{
	int fd, ret;
	struct page_table_query query = {};

	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	query.va = (__u64)(uintptr_t)va;
	ret = ioctl(fd, PAGE_TABLE_QUERY, &query);
	if (ret < 0) {
		perror("IOCTL PAGE_TABLE_QUERY failed");
		close(fd);
		return ret;
	}
	close(fd);

	print_page_table_query(&query, desc);

	return 0;
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	int ret, a_stack_variable;
	void *buffer;
	unsigned char *ch;

	buffer = mmap(NULL, sizeof(*ch), PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (buffer == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	ch = (unsigned char *)buffer;

	ret = query_va(NULL, "null");
	if (ret)
		return ret;

	ret = query_va(&query_va, "function_pointer");
	if (ret)
		return ret;
	ret = query_va(&a_global_variable, "global_variable");
	if (ret)
		return ret;
	ret = query_va(&a_stack_variable, "stack_variable");
	if (ret)
		return ret;
	ret = query_va(ch, "fresh_heap_variable");
	if (ret)
		return ret;
	sink = *ch;
	ret = query_va(ch, "read_heap_variable");
	if (ret)
		return ret;
	*ch = 0xFF;
	ret = query_va(ch, "written_heap_variable");
	if (ret)
		return ret;

	ret = munmap(buffer, sizeof(*ch));
	if (ret) {
		perror("munmap");
		return -1;
	}

	printf("Queries completed without error.\n");
	return 0;
}

/* from va-to-pa.c */
#define MOD_NAME		"PAGEMAP"
#define pr_info(fmt, ...)	fprintf(stdout, "[INFO]  " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)
#define pr_err(fmt, ...)	fprintf(stderr, "[ERROR] " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)
#define BIT_UL(nr)		(1UL << nr)
#define PM_PFRAME_BITS		55
#define PM_SOFT_DIRTY		BIT_UL(55)
#define PM_MMAP_EXCLUSIVE	BIT_UL(56)
#define PM_UFFD_WP		BIT_UL(57)
#define PM_FILE			BIT_UL(61)
#define PM_SWAP			BIT_UL(62)
#define PM_PRESENT		BIT_UL(63)
#define PAGEMAP_PATH		"/proc/self/pagemap"
static int lookup_pagemap(unsigned long *pa, unsigned long va)
{
	static unsigned long page_size = 0;

	if (page_size == 0)
		page_size = sysconf(_SC_PAGESIZE);

	int fd = open(PAGEMAP_PATH, O_RDONLY);
	if (fd == -1) {
		int retval = errno;
		pr_err("failed to open " PAGEMAP_PATH ", errmsg=%s.", strerror(errno));
		return retval;
	}

	/* the offset: each page corresponds to a 64-bit entry */
	off_t offset = (va / page_size) * 8;
	off_t real_offset = lseek(fd, offset, SEEK_SET);
	if (real_offset == -1) {
		int retval = errno;
		pr_err("failed to move file cursor, errmsg=%s.", strerror(errno));
		close(fd);
		return retval;
	}
	if (real_offset != offset) {
		pr_err("failed to complete file cursor moving. Expect %ld bytes; %ld bytes "
			"actually moved.", offset, real_offset);
		close(fd);
		return EIO;
	}

	unsigned long entry;
	int ret = read(fd, &entry, sizeof(entry));
	if (ret == -1) {
		int retval = errno;
		pr_err("failed to read PA from file " PAGEMAP_PATH ", errmsg=%s.", strerror(errno));
		close(fd);
		return retval;
	}
	if (ret != sizeof(entry)) {
		pr_err("The reading of PA is not complete. Expect %lu bytes, got %d bytes.",
			sizeof(entry), ret);
		close(fd);
		return EIO;
	}

	close(fd);

	/* decode the pagemap entry */
	/* flags: bit 55 to bit 63, 9 bits in total (some is reserved) */
	pr_info("entry=0x%lx, present=%d, swapped=%d, file-page|shared-anon=%d, "
		"userfaultfd-writeprotect=%d exclusive-map=%d, soft-dirty=%d.",
		entry, !!(entry & PM_PRESENT), !!(entry & PM_SWAP), !!(entry & PM_FILE),
		!!(entry & PM_UFFD_WP), !!(entry & PM_MMAP_EXCLUSIVE), !!(entry & PM_SOFT_DIRTY)
	);

	/* swap: 50 bits for offset, 5 bits for swap type
	 * mapped page: 55 bits for PFN */
	if (entry & PM_SWAP) {
		pr_info("\tswap-type=%lu, swap-offset=0x%lx.", entry & 0x1F,
			(entry >> 5) & ((1UL << 50) - 1));
		*pa = 0;
		return ENOMEM;
	} else {
		unsigned long pfn = entry & ((1UL << PM_PFRAME_BITS) - 1);
		pr_info("\tpfn=0x%lx.", pfn);
		*pa = pfn * page_size + va % page_size;
	}

	return 0;
}

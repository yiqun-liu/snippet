#ifndef PAGE_TABLE_QUERY_H
#define PAGE_TABLE_QUERY_H

#include <linux/types.h>
#include <linux/ioctl.h>

struct page_table_query {
	/* filled by user */
	__u32 pid;
	__u64 va;

	/* filled by kernel module */
	/* the number of translation levels walked through */
	__u32 num_levels;
	/* kernel linear addresses */
	__u64 pgd_table_kla;
	__u64 pgd_entry_kla;
	__u64 p4d_entry_kla;
	__u64 pud_entry_kla;
	__u64 pmd_entry_kla;
	__u64 pte_entry_kla;
	/* translation entry value */
	__u64 pgd_entry;
	__u64 p4d_entry;
	__u64 pud_entry;
	__u64 pmd_entry;
	__u64 pte_entry;
	/* physical address */
	__u64 pa;

	__u64 kla_start;
} __attribute__((aligned(8)));

#define PAGE_TABLE_QUERY	_IOWR('x', 0, struct page_table_query)

#endif

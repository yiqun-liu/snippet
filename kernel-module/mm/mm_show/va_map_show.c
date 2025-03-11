/*
 * A demo of address range printer.
 */

#include <linux/printk.h>
#include <linux/sizes.h>

#include "mm_show.h"

#undef pr_fmt
#define pr_fmt(fmt) "va_map_show: " fmt

static void pr_cont_size(unsigned long size)
{
	unsigned long t, g, m, k, b;
	t = size >> 40;
	g = (size & (SZ_1T - 1)) >> 30;
	m = (size & (SZ_1G - 1)) >> 20;
	k = (size & (SZ_1M - 1)) >> 10;
	b = size & (SZ_1K - 1);

	if (t)
		pr_cont("%luT.", t);
	if (g)
		pr_cont("%luG.", g);
	if (m)
		pr_cont("%luM.", m);
	if (k)
		pr_cont("%luK.", k);
	if (b)
		pr_cont("%luB.", b);
}

#define print_range(start, end) do { \
	pr_info(#start "=%#lx\t" #end "=%#lx\tlength=", start, end); \
	pr_cont_size((end) - (start)); \
} while (0)

#ifdef CONFIG_ARM64

/* arm64/memory.rst */
#include <asm/memory.h>
#include <asm/fixmap.h>
#include <asm/pgtable.h>

void va_map_show(void)
{
	pr_info("-------------- config and const. ---------------");
	pr_info("CONFIG_ARM64_VA_BITS=%d", CONFIG_ARM64_VA_BITS);
	pr_info("VA_BITS=%d, VA_BITS_MIN=%d", VA_BITS, VA_BITS_MIN);
	/* number of struct page in one page */
	pr_info("PAGE_SHIFT=%d, STRUCT_PAGE_MAX_SHIFT=%d --> VMEMMAP_SHIFT=%d",
		PAGE_SHIFT, STRUCT_PAGE_MAX_SHIFT, VMEMMAP_SHIFT);

	pr_info("--------------------  user  --------------------");

	pr_info("-------------------- kernel --------------------");
	/* bottom-up growing ranges */
	/* linear map: maximum number of physical pages */
	print_range(PAGE_OFFSET, PAGE_END);
	/* modules */
	print_range(MODULES_VADDR, MODULES_END);

	/* vmalloc: takes most the gap between bottom-up & top-down managed ranges */
	print_range(VMALLOC_START, VMALLOC_END);

	// TODO KIMAGE

	/* top-down arranged ranges */
	/* fixed mapping, used during booting */
	print_range(FIXADDR_START, FIXADDR_TOP);
	/* PCI IO */
	print_range(PCI_IO_START, PCI_IO_END);
	/* VMEMMAP */
	print_range(VMEMMAP_START, VMEMMAP_END);
	pr_info("VMEMMAP_SIZE=%#lx == ((PAGE_END - PAGE_OFFSET) >> VMEMMAP_SHIFT)\n", VMEMMAP_SIZE);

	/* flush */
	pr_cont("\n");
}

#elif CONFIG_X86_64

/* x86/x86_64/mm.rst */
/* Unlike aarch64, x86 applies a more static memory mapping assignments. See
 * arch/x86/kernel/head.c */
/* TODO learn all address markers defined in arch/x86/mm/dump_pagetables.c */
#include <linux/mm.h>
#include <asm/pgtable_64_types.h>
#include <asm/page_types.h>
#include <asm/fixmap.h>

void va_map_show(void)
{

	pr_info("-------------- config and const. ---------------");
	pr_info("SECTION_SIZE_BITS=%d", SECTION_SIZE_BITS);
	pr_info("pgtable_l5_enabled=%d", pgtable_l5_enabled());
	pr_info("MAX_PHYSMEM_BITS=%d MAXMEM=", MAX_PHYSMEM_BITS);
	pr_cont_size(MAXMEM);
	if (IS_ENABLED(CONFIG_DYNAMIC_MEMORY_LAYOUT))
		pr_info("CONFIG_DYNAMIC_MEMORY_LAYOUT=Y");
	else
		pr_info("CONFIG_DYNAMIC_MEMORY_LAYOUT=N");

	pr_info("--------------------  user  --------------------");

	pr_info("-------------------- kernel --------------------");
	print_range(GUARD_HOLE_BASE_ADDR, GUARD_HOLE_END_ADDR);
	print_range(LDT_BASE_ADDR, LDT_END_ADDR);
	/* there is no PAGE_END defined for x86. Code could check number of available pages (from
	 * memory section) to determine the upper bound of kernel linear mapping range. */
	pr_info("PAGE_OFFSET=%#lx", PAGE_OFFSET);

	print_range(VMALLOC_START, VMALLOC_END);

	print_range(FIXADDR_START, FIXADDR_TOP);
	pr_info("VMEMMAP_START=%#lx", VMEMMAP_START);

	/* flush */
	pr_cont("\n");
}

#else

void va_map_show(void)
{
	pr_info("%s: not supported in this architecture.\n", __func__);
}

#endif

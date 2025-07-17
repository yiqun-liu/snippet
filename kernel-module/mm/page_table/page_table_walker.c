// SPDX-License-Identifier: GPL-2.0
/*
 * page_table_walker.c – a demo to walk page table.
 *
 * The code work under a presumption: no device mapping is used.
 * TODO kernel page table walk
 * TODO huge zero page
 * TODO Documentation/mm/split_page_table_lock.rst
 * TODO the exported walk_page_table
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/pid.h>
#include <linux/sched.h>

#include "page_table_query.h"

#define DEVICE_NAME	"page_table_walker"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)	DEVICE_NAME ": " fmt

/* if the returned value is not NULL, always call mmput() when you finished using mm_struct. */
static struct mm_struct *get_mm_by_pid(pid_t pid)
{
	struct pid *pid_struct;
	struct task_struct *task;
	struct mm_struct *mm;

	pid_struct = find_get_pid(pid);
	if (!pid_struct) {
		pr_err("failed to get pid_struct by pid %d.\n", pid);
		return NULL;
	}

	task = get_pid_task(pid_struct, PIDTYPE_PID);
	put_pid(pid_struct);
	if (!task) {
		pr_err("failed to get task_struct by pid %d.\n", pid);
		return NULL;
	}

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm) {
		pr_err("failed to get mm_struct from the task struct of pid %d.\n", pid);
		return NULL;
	}
	return mm;
}

static int query_page_table(struct page_table_query *query)
{
	// TODO is READ_ONCE really necessary?
	// TODO why sometimes none is used, and some times none and !present are both checked?
	// TODO make sure when !present and !none, the PFN is still in place
	struct mm_struct *mm;
	/* Pointers and values of a translation descriptor are so heavily used that usually we need
	 * variables for both of them. To distinguish them the kernel has such naming conventions:
	 *  a. pointer: ptep,   entry value: pte
	 *  b. pointer: pte,    entry value: ptent
	 * here we choose the former
	 */
	pgd_t *pgdp = NULL, pgd = {};
	p4d_t *p4dp = NULL, p4d = {};
	pud_t *pudp = NULL, pud = {};
	pmd_t *pmdp = NULL, pmd = {};
	pte_t *ptep = NULL, pte = {};
	unsigned long vaddr, paddr = 0;

	if (query->pid)
		mm = get_mm_by_pid((pid_t)query->pid);
	else
		mm = get_task_mm(current);
	if (!mm) {
		pr_err("failed to get mm_struct by pid=%u.\n", query->pid);
		return -ESRCH;
	}

	vaddr = query->va;

	query->num_levels = 0;
	mmap_read_lock(mm);

	query->num_levels += 1;
	pgdp = pgd_offset(mm, vaddr);
	pgd = pgdp_get(pgdp);
	if (pgd_none(pgd) || unlikely(pgd_bad(pgd)))
		goto out;

	query->num_levels += 1;
	p4dp = p4d_offset(pgdp, vaddr);
	p4d = p4dp_get(p4dp);
	if (p4d_none(p4d) || unlikely(p4d_bad(p4d)))
		goto out;

	query->num_levels += 1;
	pudp = pud_offset(p4dp, vaddr);
	pud = pudp_get(pudp);
	if (pud_none(pud) || unlikely(pud_bad(pud)))
		goto out;
	if (pud_leaf(pud)) {
		paddr = (pud_pfn(pud) << PAGE_SHIFT) + (vaddr & ~PUD_MASK);
		goto out;
	}

	query->num_levels += 1;
	pmdp = pmd_offset(pudp, vaddr);
	pmd = pmdp_get(pmdp);
	if (pmd_none(pmd) || unlikely(pmd_bad(pmd)))
		goto out;
	if (pmd_leaf(pmd)) {
		paddr = (pmd_pfn(pmd) << PAGE_SHIFT) + (vaddr & ~PMD_MASK);
		goto out;
	}

	query->num_levels += 1;
	/* An alternative to pte_offset_kernel() (but not exported): pte_offset_map_lock. I suppose
	 * it is safe to call pte_offset_kernel here since we held mmap_read_lock.*/
	ptep = pte_offset_kernel(pmdp, vaddr);
	pte = ptep_get(ptep);
	if (pte_none(pte))
		goto out;
	paddr = (pte_pfn(pte) << PAGE_SHIFT) + (vaddr & ~PAGE_MASK);

out:
	query->pgd_table_kla = (uintptr_t)mm->pgd;

	mmap_read_unlock(mm);

	mmput(mm);

	query->pgd_entry_kla = (uintptr_t)pgdp;
	query->p4d_entry_kla = (uintptr_t)p4dp;
	query->pud_entry_kla = (uintptr_t)pudp;
	query->pmd_entry_kla = (uintptr_t)pmdp;
	query->pte_entry_kla = (uintptr_t)ptep;
	query->pgd_entry = pgd_val(pgd);
	query->p4d_entry = p4d_val(p4d);
	query->pud_entry = pud_val(pud);
	query->pmd_entry = pmd_val(pmd);
	query->pte_entry = pte_val(pte);
	query->pa = paddr;
	query->kla_start = PAGE_OFFSET;

	return 0;
}

static long walker_ioctl_unlocked(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	union {
		struct page_table_query query;
	} param;

	switch (cmd) {
	case PAGE_TABLE_QUERY:
		if (copy_from_user(&param.query, (void __user *)arg, sizeof(param.query))) {
			pr_err("failed to copy query param.\n");
			return -EFAULT;
		}
		ret = query_page_table(&param.query);
		if (ret) {
			pr_err("query failed.\n");
			return ret;
		}
		if (copy_to_user((void __user *)arg, &param.query, sizeof(param.query))) {
			pr_err("failed to copy query param.\n");
			return -EFAULT;
		}
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations walker_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = walker_ioctl_unlocked,
};

static struct miscdevice walker_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &walker_fops,
};

static int __init walker_init(void)
{
	int ret;

	ret = misc_register(&walker_misc_device);
	if (ret) {
		pr_err("misc_register failed (%d)\n", ret);
		return ret;
	}

	pr_info("kla_start = 0x%016lx.\n", PAGE_OFFSET);
	pr_info("vmalloc_start = 0x%016lx.\n", VMALLOC_START);
	pr_info("vmmemap_start = 0x%016lx.\n", VMEMMAP_START);

	/* TODO show struct */
	pr_info("misc device registered, minor=%d\n", walker_misc_device.minor);
	return 0;
}

static void __exit walker_exit(void)
{
	misc_deregister(&walker_misc_device);
	pr_info("mist device deregistered\n");
}

module_init(walker_init);
module_exit(walker_exit);

MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A demo of software page table walk");
MODULE_LICENSE("GPL");

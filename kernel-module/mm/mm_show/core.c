/*
 * A demo of kernel memory structure accessors.
 *
 * Usage:
 *   - insmod mm_show.ko [all,numa,va]
 */

#include <linux/init.h>
#include <linux/module.h>

#include "mm_show.h"

#undef pr_fmt
#define pr_fmt(fmt) "mm_show: " fmt

char default_show_target[] = "all";
static char *show_target = default_show_target;
module_param(show_target, charp, 0444);

static int __init mm_show_init(void)
{
	bool show_all;

	show_all = strcmp(show_target, "all") == 0;
	if (show_all || strcmp(show_target, "numa") == 0)
		numa_show();
	if (show_all || strcmp(show_target, "va") == 0)
		va_map_show();
	/* we do not really need the module to be plugged in */
	return -EPERM;
}

static void __exit mm_show_exit(void)
{
	return;
}

module_init(mm_show_init);
module_exit(mm_show_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A memory management info printer");

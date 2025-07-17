// SPDX-License-Identifier: GPL-2.0
/*
 * page_table_walker.c – a demo to walk page table.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

#include "page_table_query.h"

#define DEVICE_NAME	"page_table_walker"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt)	DEVICE_NAME ": " fmt

static int query_page_table(struct page_table_query *query)
{
	return -1;
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

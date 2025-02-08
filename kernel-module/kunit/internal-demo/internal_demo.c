/*
 * A demo of Linux KUnit tests for in-tree kernel modules.
 * This source file is the kernel module to be tested.
 *
 * References:
 *   - Documentation/dev-tools/kunit/start.rst
 */

#include <linux/init.h>
#include <linux/module.h>
#include "internal_demo.h"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

int demo_public_increment(int *element, int inc_value)
{
	return *element += inc_value;
}

int demo_public_decrement(int *element, int dec_value)
{
	return *element -= dec_value;
}

/* Many open-source projects just do not test static functions. */
static int demo_static_add(int a, int b)
{
	return a + b;
}

static int __init internal_demo_init(void)
{
	int temp = 0;

	/* invocate all defined functions to suppress unused warnings */
	demo_public_increment(&temp, 1);
	demo_public_decrement(&temp, 1);
	demo_static_add(1, 2);

	pr_info("In-tree component to be tested has been inserted.\n");
	return 0;
}

static void __exit internal_demo_exit(void)
{
	pr_info("In-tree component to be tested has been removed.\n");
}

module_init(internal_demo_init);
module_exit(internal_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A demo module on which to run kunit test cases");

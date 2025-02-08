/*
 * A demo of Linux KUnit tests for external modules.
 * This source file is the kernel module to be tested.
 * The test code will be dynamically linked with the source code.
 *
 * References:
 *   - Documentation/dev-tools/kunit/start.rst
 *   - Documentation/dev-tools/kunit/usage.rst
 *   - drivers/iommu/arm/arm-smmu-v3/ as reference code
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <kunit/visibility.h>
#include <kunit/static_stub.h>
#include "dynamic_demo.h"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

VISIBLE_IF_KUNIT
unsigned int demo_static_shift_left(unsigned int value, unsigned int shift)
{
	return value << shift;
}
/* exported to EXPORTED_FOR_KUNIT_TESTING symbol namespace */
EXPORT_SYMBOL_IF_KUNIT(demo_static_shift_left);

unsigned int demo_exported_shift_right(unsigned int value, unsigned int shift)
{
	return value >> shift;
}
EXPORT_SYMBOL_GPL(demo_exported_shift_right);

/* This is a static function we have interest to stub, but have no interest to test.
 * We still need to export it to make function redirection possible. */
VISIBLE_IF_KUNIT
int demo_inner_function(int param)
{
	/* name of the function and its argument(s) -- hook point for stub */
	KUNIT_STATIC_STUB_REDIRECT(demo_inner_function, param);
	pr_info("%s(%d) executed.\n", __func__, param);
	return -ENOSYS;
}
EXPORT_SYMBOL_IF_KUNIT(demo_inner_function);

VISIBLE_IF_KUNIT
int demo_outer_function(int param)
{
	int ret = demo_inner_function(param);
	if (ret < 0) {
		pr_info("%s: in NEGATIVE value branch.\n", __func__);
		return ret;
	}
	pr_info("%s: in NON-NEGATIVE value branch.\n", __func__);
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(demo_outer_function);

static int __init dynamic_demo_init(void)
{
	/* invocate all defined functions to suppress unused warnings */
	demo_static_shift_left(2, 2);
	demo_exported_shift_right(16, 2);
	demo_outer_function(1);
	pr_info("external-static module to be tested has been inserted.\n");
	return 0;
}

static void __exit dynamic_demo_exit(void)
{
	pr_info("external-static module to be tested has been removed.\n");
}

module_init(dynamic_demo_init);
module_exit(dynamic_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A dynamically linked demo module on which to run kunit test cases");

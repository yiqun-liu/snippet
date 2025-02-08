/*
 * A demo of Linux KUnit tests for external modules.
 * This source file is the kernel module to be tested.
 * The test code will be statically linked with the source code.
 *
 * References:
 *   - Documentation/dev-tools/kunit/start.rst
 *   - Documentation/dev-tools/kunit/usage.rst
 */
#include <linux/init.h>
#include <linux/module.h>
#include "static_demo.h"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* static functions CAN be tested when the test code is statically linked */
static int demo_static_multiply(int a, int b)
{
	return a * b;
}

int demo_exported_power(int base, unsigned int power)
{
	int value = 1;
	if (base == 0)
		return 0;
	while (power--)
		value *= base;
	return value;
}
EXPORT_SYMBOL_GPL(demo_exported_power);

static int __init static_demo_init(void)
{
	/* invocate all defined functions to suppress unused warnings */
	demo_static_multiply(-1, -2);
	demo_exported_power(2, 3);
	pr_info("external-static module to be tested has been inserted.\n");
	return 0;
}

static void __exit static_demo_exit(void)
{
	pr_info("external-static module to be tested has been removed.\n");
}

module_init(static_demo_init);
module_exit(static_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A statically linked demo module on which to run kunit test cases");

/* this macro should be controlled by the build scripts */
#ifdef DEMO_KUNIT_TESTS
#pragma message("NOTE: building KUNIT version for " __FILE__)
#include "tests/static_demo_test.c"
#endif

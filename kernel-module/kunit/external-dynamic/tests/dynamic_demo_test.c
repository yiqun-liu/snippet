/*
 * A demo of Linux KUnit tests for external kernel modules.
 * This source file contains the KUnit test cases and is stactically linked with the function code.
 *
 * References:
 *   - Documentation/dev-tools/kunit/usage.rst
 *   - fs/ext4/mballoc-test.c as reference code
 */

#include <kunit/test.h>
#include <kunit/static_stub.h>
#include "../dynamic_demo.h"

/* statically linked tests can test static functions in production code */
static void demo_static_shift_left_test(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 0, demo_static_shift_left(0, 3));
	KUNIT_EXPECT_EQ(test, 8, demo_static_shift_left(1, 3));
}

static void demo_exported_shift_right_test(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 0, demo_exported_shift_right(3, 2));
	KUNIT_EXPECT_EQ(test, 1, demo_exported_shift_right(3, 1));
	KUNIT_EXPECT_EQ(test, 2, demo_exported_shift_right(4, 1));
}

static int fake_demo_inner_function_abs(int param)
{
	return param < 0 ? -param : param;
}

int identity_called = 0;
static int fake_demo_inner_function_identity(int param)
{
	identity_called++;
	return param;
}

static void demo_outer_function_test(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, -ENOSYS, demo_outer_function(-10));
	KUNIT_EXPECT_EQ(test, -ENOSYS, demo_outer_function(0));
	KUNIT_EXPECT_EQ(test, -ENOSYS, demo_outer_function(10));

	kunit_activate_static_stub(test, demo_inner_function, fake_demo_inner_function_abs);
	KUNIT_EXPECT_EQ(test, 0, demo_outer_function(-10));
	KUNIT_EXPECT_EQ(test, 0, demo_outer_function(0));
	KUNIT_EXPECT_EQ(test, 0, demo_outer_function(10));
	kunit_deactivate_static_stub(test, demo_inner_function);

	kunit_activate_static_stub(test, demo_inner_function, fake_demo_inner_function_identity);
	identity_called = 0;
	KUNIT_EXPECT_EQ(test, -10, demo_outer_function(-10));
	KUNIT_EXPECT_EQ(test, 1, identity_called);
	identity_called = 0;
	KUNIT_EXPECT_EQ(test, 0, demo_outer_function(0));
	KUNIT_EXPECT_EQ(test, 1, identity_called);
	identity_called = 0;
	KUNIT_EXPECT_EQ(test, 0, demo_outer_function(10));
	KUNIT_EXPECT_EQ(test, 1, identity_called);
	kunit_deactivate_static_stub(test, demo_inner_function);
}

static struct kunit_case demo_test_cases[] = {
	KUNIT_CASE(demo_static_shift_left_test),
	KUNIT_CASE(demo_exported_shift_right_test),
	KUNIT_CASE(demo_outer_function_test),
	{}
};

static struct kunit_suite demo_test_suite = {
	.name = "external-dynamic-demo-tests",
	.test_cases = demo_test_cases,
};

kunit_test_suite(demo_test_suite);

/* import KUnit-exported symbols (the static functions to test) */
MODULE_IMPORT_NS(EXPORTED_FOR_KUNIT_TESTING);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("KUnit tests for the dynamically linked demo module");

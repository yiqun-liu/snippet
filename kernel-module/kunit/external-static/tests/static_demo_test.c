/* TODO this source, make file for production and test version, SMMU v3 for ref
 *
 * A demo of Linux KUnit tests for external kernel modules.
 * This source file contains the KUnit test cases and is stactically linked with the function code.
 *
 * References:
 *   - Documentation/dev-tools/kunit/usage.rst
 *   - fs/ext4/mballoc-test.c as reference code
 */

#include <kunit/test.h>

/* statically linked tests can test static functions in production code */
static void demo_static_multiply_test(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, -6, demo_static_multiply(2, -3));
}

static void demo_exported_power_test(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 0, demo_exported_power(0, 1));
	KUNIT_EXPECT_EQ(test, 1, demo_exported_power(3, 0));
	KUNIT_EXPECT_EQ(test, 9, demo_exported_power(3, 2));
}

static struct kunit_case demo_test_cases[] = {
	KUNIT_CASE(demo_static_multiply_test),
	KUNIT_CASE(demo_exported_power_test),
	{}
};

static struct kunit_suite demo_test_suite = {
	.name = "external-static-demo-tests",
	.test_cases = demo_test_cases,
};

kunit_test_suite(demo_test_suite);

MODULE_LICENSE("GPL");

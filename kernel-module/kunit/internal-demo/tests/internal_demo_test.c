/*
 * A demo of Linux KUnit tests for in-tree code.
 * This source file contains the KUnit test cases.
 *
 * References:
 *   - Documentation/dev-tools/kunit/start.rst
 */

#include <kunit/test.h>
#include "../internal_demo.h"

static void demo_public_increment_test(struct kunit *test)
{
	int element = 0;
	KUNIT_EXPECT_EQ(test, 1, demo_public_increment(&element, 1));
	KUNIT_EXPECT_EQ(test, 1, element);
	KUNIT_EXPECT_EQ(test, -1, demo_public_increment(&element, -2));
	KUNIT_EXPECT_EQ(test, -1, element);
}

static void demo_public_decrement_test(struct kunit *test)
{
	int element = 0;
	/* it is safe to take negation of INT_MAX, but not INT_MIN: INT_MAX + INT_MIN = -1 */
	KUNIT_EXPECT_EQ(test, -INT_MAX, demo_public_decrement(&element, INT_MAX));
	KUNIT_EXPECT_EQ(test, -INT_MAX, element);
	KUNIT_EXPECT_EQ(test, 0, demo_public_decrement(&element, -INT_MAX));
	KUNIT_EXPECT_EQ(test, 0, element);
}

static struct kunit_case demo_test_cases[] = {
	KUNIT_CASE(demo_public_increment_test),
	KUNIT_CASE(demo_public_decrement_test),
	{}
};

static struct kunit_suite demo_test_suite = {
	.name = "internal-demo-tests",
	.test_cases = demo_test_cases,
};

kunit_test_suite(demo_test_suite);

MODULE_LICENSE("GPL");

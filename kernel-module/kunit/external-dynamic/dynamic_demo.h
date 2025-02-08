#ifndef DYNAMIC_DEMO_H
#define DYNAMIC_DEMO_H

#include <linux/kconfig.h>

/*
 * A demo of Linux KUnit tests for external modules.
 * This header file contains the function prototypes of the public (non-static) functions and
 * those conditionally exported for KUnit.
 */

unsigned int demo_exported_shift_right(unsigned int value, unsigned int shift);

#if IS_ENABLED(CONFIG_KUNIT)
unsigned int demo_static_shift_left(unsigned int value, unsigned int shift);
#endif

#endif

#ifndef INTERNAL_DEMO_H
#define INTERNAL_DEMO_H
/*
 * A demo of Linux KUnit tests for in-tree kernel modules.
 * This header file contains the function prototypes of the public (non-static) functions.
 */

int demo_public_increment(int *element, int inc_value);

int demo_public_decrement(int *element, int dec_value);

#endif

#ifndef HELLO_H
#define HELLO_H

#include <stdio.h>

static inline void print_header_hello(int b) {
	if (b)
		printf("header nihao.\n");
	else
		printf("header hello.\n");
}

static inline void print_header_goodbye(void) {
	printf("header goodbye.\n");
}

#endif

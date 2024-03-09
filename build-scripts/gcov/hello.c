#include <stdio.h>
#include "hello.h"

void print_hello(int b) {
	if (b)
		printf("hello.\n");
	else
		printf("nihao.\n");
}

void print_goodbye(void) {
	printf("goodbye.\n");
}

int main(int argc, char *argv[]) {
	print_header_hello(0);
	print_hello(1);
	return 0;
}

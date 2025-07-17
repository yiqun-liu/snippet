/*
 * issue page table query request to the kernel module.
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "page_table_query.h"

#define DEVICE_PATH	  "/dev/page_table_walker"

int a_global_variable;
volatile int sink;

static int query_va(void *va, const char *desc)
{
	int fd, ret;
	struct page_table_query query = {};

	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	query.va = (__u64)(uintptr_t)va;
	ret = ioctl(fd, PAGE_TABLE_QUERY, &query);
	if (ret < 0) {
		perror("IOCTL PAGE_TABLE_QUERY failed");
		close(fd);
		return ret;
	}
	close(fd);

	/* TODO print */

	return 0;
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	int ret, a_stack_variable;
	void *buffer;
	unsigned char *ch;

	buffer = mmap(NULL, sizeof(*ch), PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (buffer == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	ch = (unsigned char *)buffer;

	ret = query_va(NULL, "null");
	if (ret)
		return ret;

	ret = query_va(&query_va, "function_pointer");
	if (ret)
		return ret;
	ret = query_va(&a_global_variable, "global_variable");
	if (ret)
		return ret;
	ret = query_va(&a_stack_variable, "stack_variable");
	if (ret)
		return ret;
	ret = query_va(ch, "fresh_heap_variable");
	if (ret)
		return ret;
	sink = *ch;
	ret = query_va(ch, "read_heap_variable");
	if (ret)
		return ret;
	*ch = 0xFF;
	ret = query_va(ch, "written_heap_variable");
	if (ret)
		return ret;

	ret = munmap(buffer, sizeof(*ch));
	if (ret) {
		perror("munmap");
		return -1;
	}

	printf("Queries completed without error.\n");
	return 0;
}

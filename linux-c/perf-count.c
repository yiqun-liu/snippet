/*
 * A demo of the C interface of linux perf (counter).
 *   1. capture the number of instructions executed over a period of time
 *   2. calculate data TLB miss rate via perf group
 *
 * References: `man perf_event_open`
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/syscall.h>

static int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
			    int group_fd, unsigned long flags)
{
	return (int)syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/* basic case: capture a single counter */
int count_instructions(unsigned long num_rounds) {
	int fd, ret;
	unsigned long value, num_instr;
	struct perf_event_attr attr = {
		.type = PERF_TYPE_HARDWARE,
		.size = sizeof(attr),
		.config = PERF_COUNT_HW_INSTRUCTIONS,
		.disabled = 1,
		.exclude_kernel = 1,
		.exclude_hv = 1,
		.remove_on_exec = 1,
	};

	/* init value */
	value = 11;
	ret = 0;

	/* prepare for the monitoring */
	fd = perf_event_open(&attr, 0, -1, -1, 0);
	if (fd == -1) {
		ret = -errno;
		fprintf(stderr, "failed to open instr. counter fd -- %s.\n",
			strerror(errno));
		goto out_close;
	}
	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

	/* run test */
	for (unsigned long i = 0; i < num_rounds; i++)
		value *= value;

	/* stop monitoring */
	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

	/* read the results */
	/* The result is actually a multi-field data structure. In this case we
	 * only care about the its first field, which is a u64 value. */
	if (read(fd, &num_instr, sizeof(num_instr)) != sizeof(num_instr)) {
		ret = -errno;
		fprintf(stderr, "read perf_result returns %s.\n", strerror(errno));
		goto out_close;
	}
	printf("0x%-8lx mult. takes 0x%-8lx instructions.\t%f instr. per mult."
		"\tresult=0x%lx\n", num_rounds, num_instr,
		(double)num_instr / (double)num_rounds, value);

out_close:
	close(fd);
	return ret;
}

#define GROUP_SIZE	2
/* the layout of perf output when PERF_FORMAT_GROUP and PERF_FORMAT_ID set as
 * attr.read_format */
struct perf_group_read_format {
	unsigned long nr;
	struct {
		unsigned long value;
		unsigned long id;
	} values[GROUP_SIZE];
};

int count_tlb_misses(unsigned long num_pages) {
	int miss_fd, access_fd, group_fd, ret = 0;
	unsigned long miss_event_id, access_event_id;
	void *pages;

	struct perf_event_attr access_attr = {
		.type = PERF_TYPE_HW_CACHE,
		.size = sizeof(access_attr),
		.config = (PERF_COUNT_HW_CACHE_DTLB) |
			(PERF_COUNT_HW_CACHE_OP_READ << 8) |
			(PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
		.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID,
		.disabled = 1,
		.exclude_kernel = 1,
		.exclude_hv = 1,
		.remove_on_exec = 1,
	};
	struct perf_event_attr miss_attr = {
		.type = PERF_TYPE_HW_CACHE,
		.size = sizeof(miss_attr),
		.config = (PERF_COUNT_HW_CACHE_DTLB) |
			(PERF_COUNT_HW_CACHE_OP_READ << 8) |
			(PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
		.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID,
		.disabled = 1,
		.exclude_kernel = 1,
		.exclude_hv = 1,
		.remove_on_exec = 1,
	};

	/* prepare the data */
	pages = mmap(NULL, num_pages * getpagesize(), PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (pages == MAP_FAILED) {
		fprintf(stderr, "failed to map 0x%lx pages.\n", num_pages);
		return -ENOMEM;
	}
	srandom(0);

	/* prepare for the monitoring */
	/* creat a group leader fd by passing in -1 as group_fd */
	access_fd = perf_event_open(&access_attr, 0, -1, -1, 0);
	group_fd = access_fd;
	/* pass the group leader fd (in this case, the access_fd) in as group_fd */
	miss_fd = perf_event_open(&miss_attr, 0, -1, group_fd, 0);

	if (access_fd == -1 || miss_fd == -1) {
		fprintf(stderr, "failed to open tlb count or miss fd -- %s.\n",
			strerror(errno));
		ret = -1;
		goto out_unmap;
	}
	/* to parse the grouped results, we need to read the id of each event */
	ioctl(access_fd, PERF_EVENT_IOC_ID, &access_event_id);
	ioctl(miss_fd, PERF_EVENT_IOC_ID, &miss_event_id);

	/* start monitor */
	ioctl(group_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
	ioctl(group_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);

	/* run test: 16x page access */
	int result = 0;
	for (unsigned long i = 0; i < 16 * num_pages; i++) {
		unsigned long page_id = random() % num_pages;
		const char *p = (const char*)(pages) + page_id * getpagesize();
		result += *p;
	}

	ioctl(group_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

	/* read and parse result */
	struct perf_group_read_format perf_result = {};
	if (read(group_fd, &perf_result, sizeof(perf_result)) != sizeof(perf_result)) {
		ret = -errno;
		fprintf(stderr, "read perf_result returns %s.\n", strerror(errno));
		goto out_close;
	}

	unsigned long dtlb_miss, dtlb_access;
	if (perf_result.values[0].id == access_event_id &&
		perf_result.values[1].id == miss_event_id) {
		dtlb_access = perf_result.values[0].value;
		dtlb_miss = perf_result.values[1].value;
	} else if (perf_result.values[1].id == access_event_id &&
		perf_result.values[0].id == miss_event_id) {
		dtlb_access = perf_result.values[1].value;
		dtlb_miss = perf_result.values[0].value;
	} else {
		fprintf(stderr, "Parse error: event id does not match.\n");
		goto out_close;
	}

	printf("0x%-8lx pages:\t0x%-8lx miss on 0x%-8lx random accesses.\t"
		"miss-rate=%8f%%\tresult=%d.\n", num_pages, dtlb_miss,
		dtlb_access, 100. * (double)dtlb_miss / (double)dtlb_access,
		result);

out_close:
	close(miss_fd);
	close(access_fd);
out_unmap:
	munmap(pages, num_pages * getpagesize());

	return ret;
}

int main(void)
{
	int ret = 0;

	printf("[TEST 1] instruction count.\n");
	unsigned long num_rounds = 0x800000UL;
	for (unsigned long i = 1; i <= 4 && ret == 0; i++)
		ret = count_instructions(i * num_rounds);

	printf("\n[TEST 2] tlb miss count.\n");
	unsigned long num_pages = 0x200000UL;
	for (unsigned long i = 1; i <= 4 && ret == 0; i++)
		ret = count_tlb_misses(i * num_pages);

	if (ret)
		fprintf(stderr, "failed to run perf-count demo.\n");
	return ret;
}

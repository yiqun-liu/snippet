/*
 * A demo of /proc/$PID/pagemap: address translation in a user space process.
 *
 * Reference: kernel doc vm/pagemap.txt
 *
 * Check /proc/$PID/maps as a cross validation
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define MOD_NAME		"addr-translator"
#define pr_info(fmt, ...)	fprintf(stdout, "[INFO]  " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)
#define pr_err(fmt, ...)	fprintf(stderr, "[ERROR] " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)

unsigned long g_page_size;
pid_t g_pid;

/* adapted from kernel source: fs/proc/task_mmu.c or vm/pagemap.rst */
#define BIT_UL(nr)		(1UL << nr)

#define PM_PFRAME_BITS		55
#define PM_SOFT_DIRTY		BIT_UL(55)
#define PM_MMAP_EXCLUSIVE	BIT_UL(56)
#define PM_UFFD_WP		BIT_UL(57)
#define PM_FILE			BIT_UL(61)
#define PM_SWAP			BIT_UL(62)
#define PM_PRESENT		BIT_UL(63)

static int translate_address(unsigned long *pa, unsigned long va)
{
	char file_path[64];
	sprintf(file_path, "/proc/%d/pagemap", g_pid);

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		int retval = errno;
		pr_err("failed to open file %s, errmsg=%s.", file_path, strerror(errno));
		return retval;
	}

	/* the offset: each page corresponds to a 64-bit entry */
	off_t offset = (va / g_page_size) * 8;
	off_t real_offset = lseek(fd, offset, SEEK_SET);
	if (real_offset == -1) {
		int retval = errno;
		pr_err("failed to move file cursor, errmsg=%s.", strerror(errno));
		close(fd);
		return retval;
	}
	if (real_offset != offset) {
		pr_err("failed to complete file cursor moving. Expect %ld bytes; %ld bytes "
			"actually moved.", offset, real_offset);
		close(fd);
		return EIO;
	}

	unsigned long entry;
	int ret = read(fd, &entry, sizeof(entry));
	if (ret == -1) {
		int retval = errno;
		pr_err("failed to read PA from file %s, errmsg=%s.", file_path, strerror(errno));
		close(fd);
		return retval;
	}
	if (ret != sizeof(entry)) {
		pr_err("The reading of PA is not complete. Expect %lu bytes, got %d bytes.",
			sizeof(entry), ret);
		close(fd);
		return EIO;
	}

	close(fd);

	/* decode the pagemap entry */
	/* flags: bit 55 to bit 63, 9 bits in total (some is reserved) */
	pr_info("entry=0x%lx, present=%d, swapped=%d, file-page|shared-anon=%d, "
		"userfaultfd-writeprotect=%d exclusive-map=%d, soft-dirty=%d.",
		entry, !!(entry & PM_PRESENT), !!(entry & PM_SWAP), !!(entry & PM_FILE),
		!!(entry & PM_UFFD_WP), !!(entry & PM_MMAP_EXCLUSIVE), !!(entry & PM_SOFT_DIRTY)
	);

	/* swap: 50 bits for offset, 5 bits for swap type
	 * mapped page: 55 bits for PFN */
	if (entry & PM_SWAP) {
		pr_info("\tswap-type=%lu, swap-offset=0x%lx.", entry & 0x1F,
			(entry >> 5) & ((1UL << 50) - 1));
		*pa = 0;
		return ENOMEM;
	} else {
		unsigned long pfn = entry & ((1UL << PM_PFRAME_BITS) - 1);
		pr_info("\tpfn=0x%lx.", pfn);
		*pa = pfn * g_page_size + va % g_page_size;
	}

	return 0;
}

void sigint_hanlder(int sig)
{
	pr_info("signal %d received.", sig);
}

int main(void)
{
	g_page_size = sysconf(_SC_PAGESIZE);
	g_pid = getpid();

	/* create a valid mapping */
	void *buffer = mmap(NULL, g_page_size, PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (buffer == MAP_FAILED) {
		int retval = errno;
		pr_err("failed to mmap a page, errmsg=%s.", strerror(errno));
		return retval;
	}
	pr_info("buffer allocated for process %d: VA=%p.", g_pid, buffer);
	fflush(stdout);

	unsigned long pa;
	int ret = translate_address(&pa, (unsigned long)buffer);
	if (ret)
		pr_info("before access: translation failed.");
	else
		pr_info("before access: VA=%p, PA=0x%lx.", buffer, pa);

	/* trigger demand paging */
	memset(buffer, 0, g_page_size);

	ret = translate_address(&pa, (unsigned long)buffer);
	if (ret)
		pr_info("after access: translation failed.");
	else
		pr_info("after access: VA=%p, PA=0x%lx.", buffer, pa);

	/* offer user a chance to check /proc/$PID/maps */
	pr_info("pause process %d. Interrupt it with SIGINT", g_pid);

	if (signal(SIGINT, sigint_hanlder) == SIG_ERR) {
		int retval = errno;
		pr_err("failed to register custom sigint handler, errmsg=%s.", strerror(errno));
		return retval;
	}
	pause();

	munmap(buffer, g_page_size);

	/* the return value of the after-access translation is returned. */
	return ret;
}

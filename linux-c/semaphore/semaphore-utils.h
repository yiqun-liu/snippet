/*
 * This header provides simple semaphore utilities wrappers.
 * To simplify error handling, all wrappers in this file terminate the process on failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

/* the name should starts with '/', see `man sem_overview` */
#define SEM_NAME	"/demo-semaphore"
#define SEM_INIT_VALUE	(0)

static void print_msg(const char *fmt, ...)
{
	struct timespec spec;
	int ret = clock_gettime(CLOCK_REALTIME, &spec);
	if (ret == -1) {
		int retval = errno;
		perror("failed to get time.");
		exit(retval);
	}

	struct tm *p_tm = localtime(&spec.tv_sec);
	if (p_tm == NULL) {
		int retval = errno;
		perror("failed to get localized time.");
		exit(retval);
	}

	/* h, m, s, us, pid */
	printf("[%2d:%2d:%2d:%3ld](%d) ", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec,
		spec.tv_nsec / 1000, getpid());

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

static void* get_shm(size_t size)
{
	int fd = shm_open(SEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		int retval = errno;
		fprintf(stderr, "failed to open_shm, name=%s, oflag=0x%x, mode=0x%x\n"
			"errmsg='%s'.\n", SEM_NAME, O_CREAT | O_EXCL | O_RDWR,
			S_IRUSR | S_IWUSR, strerror(errno));
		exit(retval);
	}

	int ret = ftruncate(fd, size);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to ftruncate, fd=%d, size=0x%lx\n"
			"errmsg='%s'.\n", fd, size, strerror(errno));
		exit(retval);
	}

	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		int retval = errno;
		fprintf(stderr, "failed to mmap, addr=%p, size=0x%zx, prot=0x%x, flags=0x%x, "
			"fd=%d, off=0x%x\nerrmsg='%s'.\n", NULL, size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0, strerror(errno));
		exit(retval);
	}

	ret = close(fd);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to close, fd=%d\nerrmsg='%s'.\n",
			fd, strerror(errno));
		exit(retval);
	}

	return ptr;
}

static void put_shm(void *ptr, size_t size)
{
	int ret = munmap(ptr, size);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to munmap, ptr=%p, size=0x%lx\n"
			"errmsg='%s'.\n", ptr, size, strerror(errno));
		exit(retval);
	}

	ret = shm_unlink(SEM_NAME);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to shm_unlink, name=%s\nerrmsg='%s'.\n",
			SEM_NAME, strerror(errno));
		exit(retval);
	}
}

static sem_t* create_semaphore(void)
{
	sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, SEM_INIT_VALUE);
	if (sem == SEM_FAILED) {
		int retval = errno;
		fprintf(stderr, "failed to create semaphore, name=%s, oflag=0x%x, mode=0x%x, "
			"init_val=%d\nerrmsg='%s'.\n", SEM_NAME, O_CREAT | O_EXCL,
			S_IRUSR | S_IWUSR, SEM_INIT_VALUE, strerror(errno));
		exit(retval);
	}
	return sem;
}

static void unlink_semaphore(void)
{
	int ret = sem_unlink(SEM_NAME);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to unlink semaphore, name=%s\nerrmsg='%s'.\n",
			SEM_NAME, strerror(errno));
		exit(retval);
	}
}

static void init_semaphore(sem_t *sem, int pshared)
{
	int ret = sem_init(sem, pshared, SEM_INIT_VALUE);
	printf("initialize semaphore, sem=%p, pshared=%d, value=%d. ret=%d\n",
		sem, pshared, SEM_INIT_VALUE, ret);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to initialize semaphore, sem=%p, pshared=%d, value=%d.\n"
			"errmsg='%s'.\n", sem, pshared, SEM_INIT_VALUE, strerror(errno));
		exit(retval);
	}
}

static void destroy_semaphore(sem_t *sem)
{
	int ret = sem_destroy(sem);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to destroy semaphore, sem=%p.\nerrmsg='%s'.\n",
			&sem, strerror(errno));
		exit(retval);
	}
}

static sem_t* open_semaphore(void)
{
	/* NOTE: two arguments fewer than the invocation in sem_open */
	sem_t *sem = sem_open(SEM_NAME, 0);
	if (sem == SEM_FAILED) {
		int retval = errno;
		fprintf(stderr, "failed to open semaphore, name=%s, oflag=0.\nerrmsg='%s'.\n",
			SEM_NAME, strerror(errno));
		exit(retval);
	}
	return sem;
}

static void close_semaphore(sem_t *sem)
{
	int ret = sem_close(sem);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to close semaphore, ptr=%p\nerrmsg='%s'.\n",
			sem, strerror(errno));
		exit(retval);
	}
}

static void post_semaphore(sem_t *sem)
{
	int ret = sem_post(sem);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to post semaphore, ptr=%p\nerrmsg='%s'.\n",
			sem, strerror(errno));
		exit(retval);
	}
}

static void wait_semaphore(sem_t *sem)
{
	int ret = sem_wait(sem);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to wait semaphore, ptr=%p\nerrmsg='%s'.\n",
			sem, strerror(errno));
		exit(retval);
	}
}

static void get_semaphore(sem_t *sem)
{
	int value;
	int ret = sem_getvalue(sem, &value);
	if (ret == -1) {
		int retval = errno;
		fprintf(stderr, "failed to get semaphore, ptr=%p\nerrmsg='%s'.\n",
			sem, strerror(errno));
		exit(retval);
	}
	print_msg("semaphore value = %d.\n", value);
}

/* A demo of named semaphore (POSIX) */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "semaphore-utils.h"

void print_usage(void)
{
	printf("Usage: ./named-semaphore command\n"
		"Available commands: create, unlink, post, wait, help.\n");
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Invalid argument count %d.\n", argc);
		print_usage();
		return EINVAL;
	}

	const char *cmd = argv[1];

	print_msg("%s start.\n", cmd);
	if (strcmp(cmd, "create") == 0) {
		sem_t *sem = create_semaphore();
		close_semaphore(sem);
	} else if (strcmp(cmd, "unlink") == 0) {
		unlink_semaphore();
	} else if (strcmp(cmd, "post") == 0) {
		sem_t *sem = open_semaphore();
		post_semaphore(sem);
		close_semaphore(sem);
	} else if (strcmp(cmd, "wait") == 0) {
		sem_t *sem = open_semaphore();
		wait_semaphore(sem);
		close_semaphore(sem);
	} else if (strcmp(cmd, "get") == 0) {
		sem_t *sem = open_semaphore();
		get_semaphore(sem);
		close_semaphore(sem);
	} else if (strcmp(cmd, "help") == 0) {
		print_usage();
	} else {
		print_usage();
		return EINVAL;
	}
	print_msg("%s done.\n", cmd);

	return 0;
}

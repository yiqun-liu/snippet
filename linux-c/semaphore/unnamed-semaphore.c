/*
 * A demo of unnamed semaphore (POSIX)
 *
 * This program demonstrates a bidirectional producer-consumer scenario:
 * 1st round: the parent is the producer, and the child waits for the parent;
 * 2nd round: the child is the producer, and the parent waits for the child.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "semaphore-utils.h"

int main(void)
{
	sem_t *parent_sem, *child_sem;
	const size_t shm_size = sizeof(sem_t) * 2;
	/* for semaphores to synchronizes processes, it must be allocated from share memory */
	void * const buffer = get_shm(shm_size);

	parent_sem = (sem_t*)buffer;
	child_sem = ((sem_t*)buffer) + 1;

	/* initialize semaphores: process-sharing=true. */
	init_semaphore(parent_sem, 1);
	init_semaphore(child_sem, 1);

	int pid = fork();
	if (pid == -1) {
		/* fork failed */
		int retval = errno;
		fprintf(stderr, "failed to fork.\nerrmsg='%s'.\n", strerror(errno));
		exit(retval);
	} else if (pid != 0) {
		/* parent */
		print_msg("(1st round) parent produced.\n");
		sleep(1);
		post_semaphore(parent_sem);

		/*
		 * We cannot directly use the same semaphore, otherwise we might decrease the
		 * semaphore and undo the effects we just increased (then, no **inter**-
		 * process synchronization happens).
		 *
		 * The fix is to use two semaphores. Another workaround is to poll the semaphore
		 * with `getvalue()` ... but that is against the spirit of semaphore, and we'd
		 * better use shared memory in that case.
		 */
		print_msg("(2nd round) parent consumes (wait) ...\n");
		wait_semaphore(child_sem);
		print_msg("(2nd round) parent consumed ...\n");
	} else {
		/* child */
		print_msg("(1st round) child consumes (wait) ...\n");
		wait_semaphore(parent_sem);
		print_msg("(1st round) child consumed.\n");

		print_msg("(2nd round) child produced.\n");
		post_semaphore(child_sem);
		return 0;
	}

	/* only parent reaches here */
	int ret, status;
	ret = waitpid(pid, &status, 0);
	if (ret != pid) {
		int retval = errno;
		fprintf(stderr, "failed to wait for child %d. errmsg='%s'\n", pid, strerror(errno));
		exit(retval);
	}
	if (!WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		int retval = errno;
		fprintf(stderr, "child %d did not terminate properly. is_exit=%d, exit_status=%d\n"
			"errmsg='%s'\n",
			pid, (int)WIFEXITED(status), WEXITSTATUS(status), strerror(errno));
		exit(retval);
	}

	destroy_semaphore(parent_sem);
	destroy_semaphore(child_sem);

	put_shm(buffer, shm_size);

	return 0;
}

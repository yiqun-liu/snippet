/*
 * A demo of Linux Semaphore
 *
 * References:
 *   - LKD 3 ed. chapter 10
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/semaphore.h>

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define NUM_WORKERS 6

#define RACER_WAIT 0
#define RACER_RACE 1
#define RACER_ABORT 2

struct {
	/*
	 * In this demo, the semaphore is intialized with sema_init(). It is also valid to use the
	 * static initializer "DEFINE_SEMAPHORE". Since 48380368dec14 (2023), the interface no
	 * longer assumes binary semaphore and takes two arguments: name, capacity. Before that the
	 * capacity is always assumed to be 1.
	 */
	struct semaphore sem;

	atomic_t ready_count;
	atomic_t holder_count;
	volatile int action;
} g_demo_ctx;

/* a kthread function */
static int do_race(void *data)
{
	int ret, current_ready_count, current_holder_count;
	unsigned long id;

	ret = 0;
	id = (unsigned long)data;

	current_ready_count = atomic_inc_return(&g_demo_ctx.ready_count);
	pr_info("Racer-%lu: %d racers ready.\n", id, current_ready_count);
	if (current_ready_count == NUM_WORKERS) {
		pr_info("All %d racers ready. Start racing.\n", NUM_WORKERS);
		g_demo_ctx.action = 1;
	}
	while (g_demo_ctx.action == RACER_WAIT)
		;
	if (g_demo_ctx.action == RACER_ABORT) {
		pr_info("Racer-%lu received abort instruction.\n", id);
		ret = -1;
		goto out_put;
	}

	/* hold the lock, not interruptible (there are _interruptible, _killable, _trylock and
	 * _timeout variants) */
	down(&g_demo_ctx.sem);
	/* start of the critical section */
	current_holder_count = atomic_inc_return(&g_demo_ctx.holder_count);
	if (current_holder_count != 1) {
		pr_err("Concurrency invariant violated: %d threads in critical section!\n",
			current_holder_count);
		ret = -1;
	} else {
		pr_info("Racer-%lu enters critical section. Everything seems OK.\n", id);
	}

	/* the thread may sleep holding a semaphore */
	msleep(100);
	pr_info("Racer-%lu returns from sleep.\n", id);

	atomic_dec(&g_demo_ctx.holder_count);
	/* end of the critical section */
	up(&g_demo_ctx.sem);

out_put:
	module_put(THIS_MODULE);
	return ret;
}

static int __init demo_semaphore_init(void)
{
	unsigned long i;

	atomic_set(&g_demo_ctx.ready_count, 0);
	atomic_set(&g_demo_ctx.holder_count, 0);
	sema_init(&g_demo_ctx.sem, 1);
	g_demo_ctx.action = RACER_WAIT;

	for (i = 0; i < NUM_WORKERS; i++) {
		struct task_struct *task;
		__module_get(THIS_MODULE);
		task = kthread_run(do_race, (void*)i, "demo-racer-%lu", i);
		if (IS_ERR(task)) {
			module_put(THIS_MODULE);
			pr_err("failed to start kthread 'Racer-%lu'.\n", i);
			g_demo_ctx.action = RACER_ABORT;
			return -1;
		}
	}
	pr_info("All racer threads scheduled.\n");
	g_demo_ctx.action = RACER_RACE;
	return 0;
}

static void __exit demo_semaphore_exit(void)
{
}

module_init(demo_semaphore_init);
module_exit(demo_semaphore_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A demo of Linux semaphore");

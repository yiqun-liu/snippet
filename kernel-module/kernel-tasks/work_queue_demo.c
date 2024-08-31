/*
 * A demo of Linux Work Queue
 *
 * References:
 *   - LKD 3 ed. chapter 8
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* static work */
static void static_work_func(struct work_struct *work)
{
	pr_info("static work executed, work=%px.\n", work);
	module_put(THIS_MODULE);
}
DECLARE_WORK(static_work, static_work_func);

static void schedule_static_work(void)
{
	if (!try_module_get(THIS_MODULE)) {
		pr_err("%s: failed to get module.\n", __func__);
		return;
	}
	if (!schedule_work(&static_work)) {
		pr_err("failed to schedule static work (duplicate).\n");
		module_put(THIS_MODULE);
	}
	pr_info("static work %px scheduled.\n", &static_work);
}

/* dynamic work */
/* work_struct being embedded in a larger struct is the common practice. Tasks can get information
 * from both the fields of the enclosing struct. The embedding is not so convenient with current
 * static API, so we demostrate its usage as dynamic work only. */
struct my_dynamic_work {
	struct work_struct work;
	const char *name;
};
static void dynamic_work_func(struct work_struct *work)
{
	struct my_dynamic_work *my_dynamic_work;
	my_dynamic_work = container_of(work, struct my_dynamic_work, work);
	pr_info("dynamic worker executed, work=%px, name=%s.\n", work, my_dynamic_work->name);
	kfree(my_dynamic_work);
	module_put(THIS_MODULE);
}

static void schedule_dynamic_work(void)
{
	/* initialize dynamic work */
	/* NOTE: The task usually outlives its scheduling function -- schedule_dynamic_work() in
	 * this demo. Therefore we generally cannot place its control data structue on stack. */
	struct my_dynamic_work *my_dynamic_work = (struct my_dynamic_work*)kmalloc(
		sizeof(struct my_dynamic_work), GFP_KERNEL);
	my_dynamic_work->name = "my_dynamic_work";
	INIT_WORK(&my_dynamic_work->work, dynamic_work_func);

	/* schedule dynamic work */
	if (!try_module_get(THIS_MODULE)) {
		pr_err("%s: failed to get module.\n", __func__);
		goto out_free;
	}
	if (!schedule_work(&my_dynamic_work->work)) {
		pr_err("failed to schedule dynamic work (duplicate).\n");
		goto out_put;
	}
	pr_info("dynamic work %px scheduled.\n", &my_dynamic_work->work);
	return;

out_put:
	module_put(THIS_MODULE);
out_free:
	kfree(my_dynamic_work);
}

static int __init demo_workqueue_init(void)
{
	schedule_static_work();
	schedule_dynamic_work();
	return 0;
}

static void __exit demo_workqueue_exit(void)
{
}

module_init(demo_workqueue_init);
module_exit(demo_workqueue_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A demo of Linux work queue");

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <uapi/asm-generic/errno-base.h>

#ifdef CONFIG_NUMA
#include <linux/numa.h>
#include <linux/mempolicy.h>
#endif

#define DEVICE_NAME		 "memdev"
#define DEVICE_PATH		 "/dev/memdev"

enum prot_type {
	PROT_CACHEABLE		= 0,
	PROT_WRITECOMBINE	= 1,
	PROT_UNCACHED		= 2,
};

struct mapped_region {
	pid_t pid;
	unsigned long user_va;
	size_t size;
	struct page **pages;
	unsigned long npages;
	struct list_head list;
};

struct file_state {
	struct list_head regions;
	struct list_head list;
};

static char *memdev_attr = "normal_cacheable";
module_param_named(attr, memdev_attr, charp, 0644);
MODULE_PARM_DESC(attr, "Memory attribute: normal_cacheable, normal_noncacheable, device_noncacheable");

static enum prot_type g_prot;

static dev_t devno;
static struct cdev memdev_cdev;
static struct class *memdev_class;
static struct device *memdev_device;

static LIST_HEAD(file_list);
static DEFINE_MUTEX(dev_lock);

static atomic_t mapped_count = ATOMIC_INIT(0);

#ifdef CONFIG_NUMA
static bool numa_enabled;
#endif

static int get_allocation_node(struct mempolicy *policy)
{
	int mode = policy ? policy->mode : MPOL_DEFAULT;

	switch (mode) {
	case MPOL_DEFAULT:
	case MPOL_LOCAL:
	case MPOL_PREFERRED:
		return cpu_to_node(get_cpu());
	default:
		return -EINVAL;
	}
}

static pgprot_t get_pgprot(enum prot_type prot)
{
	pgprot_t prot_val = vm_get_page_prot(VM_READ | VM_WRITE);
	switch (prot) {
	case PROT_WRITECOMBINE:
		return pgprot_writecombine(prot_val);
	case PROT_UNCACHED:
		return pgprot_noncached(prot_val);
	case PROT_CACHEABLE:
		/* fallthrough */
	default:
		return prot_val;
	}
}

static void free_mapped_region(struct mapped_region *region)
{
	unsigned long i;

	for (i = 0; i < region->npages; i++) {
		__free_page(region->pages[i]);
		if ((i + 1) % 256 == 0)
			cond_resched();
	}
	kvfree(region->pages);
	kfree(region);
	atomic_dec(&mapped_count);
}

static int map_pages_to_vma(struct vm_area_struct *vma, struct page **pages,
			    size_t size, pgprot_t prot)
{
	unsigned long npages = size / PAGE_SIZE;
	unsigned long npages_unmap = npages;
	int ret;

	vma->vm_page_prot = prot;
	ret = vm_insert_pages(vma, vma->vm_start, pages, &npages_unmap);
	if (ret)
		pr_err("%lu of %lu pages failed to be mapped.\n", npages_unmap, npages);

	return ret;
}

static void memdev_vma_close(struct vm_area_struct *vma)
{
	struct file_state *state;
	struct mapped_region *region, *tmp;
	pid_t pid = task_pid_nr(current->group_leader);
	unsigned long user_va = vma->vm_start;
	int found = 0;

	mutex_lock(&dev_lock);

	list_for_each_entry(state, &file_list, list) {
		list_for_each_entry_safe(region, tmp, &state->regions, list) {
			if (region->pid == pid && region->user_va == user_va) {
				pr_info("freeing region pid=%d va=%#lx size=%#zx\n",
					pid, user_va, region->size);
				list_del(&region->list);
				free_mapped_region(region);
				found = 1;
				goto out;
			}
		}
	}

	if (!found)
		pr_warn("region not found pid=%d va=%#lx\n", pid, user_va);

out:
	mutex_unlock(&dev_lock);
}

static const struct vm_operations_struct memdev_vm_ops = {
	.close = memdev_vma_close,
};

static int memdev_open(struct inode *inode, struct file *filp)
{
	struct file_state *state;

	if (!try_module_get(THIS_MODULE))
		return -EBUSY;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state) {
		module_put(THIS_MODULE);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&state->regions);
	INIT_LIST_HEAD(&state->list);

	mutex_lock(&dev_lock);
	list_add(&state->list, &file_list);
	mutex_unlock(&dev_lock);

	filp->private_data = state;

	return 0;
}

static int memdev_release(struct inode *inode, struct file *filp)
{
	struct file_state *state = filp->private_data;
	struct mapped_region *region, *tmp;

	if (state) {
		mutex_lock(&dev_lock);
		list_for_each_entry_safe(region, tmp, &state->regions, list) {
			list_del(&region->list);
			free_mapped_region(region);
		}
		list_del(&state->list);
		mutex_unlock(&dev_lock);

		kfree(state);
	}

	module_put(THIS_MODULE);
	return 0;
}

static int memdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct file_state *state = filp->private_data;
	struct mapped_region *region;
	size_t size = vma->vm_end - vma->vm_start;
	struct page **pages;
	unsigned long npages, nr_allocated;
	int ret, node;
	pid_t pid;

	if (size == 0 || (size % PAGE_SIZE) != 0) {
		pr_err("invalid size %zu\n", size);
		return -EINVAL;
	}

	npages = size / PAGE_SIZE;
	node = cpu_to_node(get_cpu());

#ifdef CONFIG_NUMA
	if (numa_enabled) {
		struct mempolicy *policy = vma->vm_policy;

		node = get_allocation_node(policy);
		if (node < 0) {
			pr_err("mmap rejected: unsupported mempolicy (mode=%d)\n",
				policy ? policy->mode : MPOL_DEFAULT);
			return -EOPNOTSUPP;
		}
	}
#endif

	pages = kvmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		pr_err("failed to allocate page array\n");
		return -ENOMEM;
	}

	nr_allocated = 0;
	nr_allocated = alloc_pages_bulk_array_node(GFP_KERNEL, node, npages, pages);
	if (nr_allocated != npages) {
		pr_err("bulk allocation failed: got %lu of %lu pages\n",
			nr_allocated, npages);
		ret = -ENOMEM;
		goto err_free_pages;
	}

	ret = map_pages_to_vma(vma, pages, size, get_pgprot(g_prot));
	if (ret) {
		pr_err("failed to map pages: %d\n", ret);
		goto err_free_pages;
	}

	region = kzalloc(sizeof(*region), GFP_KERNEL);
	if (!region) {
		pr_err("failed to allocate region\n");
		ret = -ENOMEM;
		goto err_free_pages;
	}

	pid = task_pid_nr(current->group_leader);
	region->pid = pid;
	region->user_va = vma->vm_start;
	region->size = size;
	region->pages = pages;
	region->npages = npages;
	INIT_LIST_HEAD(&region->list);

	mutex_lock(&dev_lock);
	list_add(&region->list, &state->regions);
	mutex_unlock(&dev_lock);

	atomic_inc(&mapped_count);

	vma->vm_private_data = region;
	vma->vm_ops = &memdev_vm_ops;

	pr_info("mapped region pid=%d va=%#lx size=%#zx node=%d\n",
		pid, region->user_va, region->size, node);
	return 0;

err_free_pages:
	while (nr_allocated-- > 0)
		__free_page(pages[nr_allocated]);
	kvfree(pages);
	return ret;
}

static long memdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static const struct file_operations memdev_fops = {
	.owner		= THIS_MODULE,
	.open		= memdev_open,
	.release	= memdev_release,
	.mmap		= memdev_mmap,
	.unlocked_ioctl	= memdev_ioctl,
	.compat_ioctl	= memdev_ioctl,
};

static int __init memdev_init(void)
{
	int ret;

#ifdef CONFIG_NUMA
	numa_enabled = (num_possible_nodes() > 1);
#else
	numa_enabled = false;
#endif

	if (strcmp(memdev_attr, "normal_cacheable") == 0) {
		g_prot = PROT_CACHEABLE;
	} else if (strcmp(memdev_attr, "normal_noncacheable") == 0) {
		g_prot = PROT_WRITECOMBINE;
	} else if (strcmp(memdev_attr, "device_noncacheable") == 0) {
		g_prot = PROT_UNCACHED;
	} else {
		pr_err("invalid attr '%s'\n", memdev_attr);
		return -EINVAL;
	}

	ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_err("failed to allocate device number\n");
		return ret;
	}

	cdev_init(&memdev_cdev, &memdev_fops);
	memdev_cdev.owner = THIS_MODULE;

	ret = cdev_add(&memdev_cdev, devno, 1);
	if (ret < 0) {
		pr_err("failed to add cdev\n");
		goto err_cdev_add;
	}

	memdev_class = class_create(DEVICE_NAME);
	if (IS_ERR(memdev_class)) {
		pr_err("failed to create class\n");
		ret = PTR_ERR(memdev_class);
		goto err_class_create;
	}

	memdev_device = device_create(memdev_class, NULL, devno, NULL, DEVICE_NAME);
	if (IS_ERR(memdev_device)) {
		pr_err("failed to create device\n");
		ret = PTR_ERR(memdev_device);
		goto err_device_create;
	}

	pr_info("initialized, device at %s (attr=%s, numa=%s, mapped_count=%d)\n",
		DEVICE_PATH, memdev_attr, numa_enabled ? "enabled" : "disabled",
		atomic_read(&mapped_count));
	return 0;

err_device_create:
	class_destroy(memdev_class);
err_class_create:
	cdev_del(&memdev_cdev);
err_cdev_add:
	unregister_chrdev_region(devno, 1);
	return ret;
}

static void __exit memdev_exit(void)
{
	struct file_state *state, *tmp_state;
	struct mapped_region *region, *tmp;

	if (atomic_read(&mapped_count) > 0) {
		pr_warn("refusing unload, %d mappings still active but the device unloaded.\n",
			atomic_read(&mapped_count));
		return;
	}

	mutex_lock(&dev_lock);
	list_for_each_entry_safe(state, tmp_state, &file_list, list) {
		list_for_each_entry_safe(region, tmp, &state->regions, list) {
			list_del(&region->list);
			free_mapped_region(region);
		}
		list_del(&state->list);
		kfree(state);
	}
	mutex_unlock(&dev_lock);

	device_destroy(memdev_class, devno);
	class_destroy(memdev_class);
	cdev_del(&memdev_cdev);
	unregister_chrdev_region(devno, 1);
	pr_info("unloaded\n");
}

module_init(memdev_init);
module_exit(memdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu");
MODULE_DESCRIPTION("Memory device with configurable cache attributes");
MODULE_VERSION("0.1");

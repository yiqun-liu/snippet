#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <uapi/asm-generic/errno-base.h>

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
	void *kernel_va;
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
	if (region->kernel_va)
		vfree(region->kernel_va);
	kfree(region);
	atomic_dec(&mapped_count);
}

static int map_vmalloc_range(struct vm_area_struct *vma, void *addr,
				 size_t size, pgprot_t prot)
{
	unsigned long i, npages, npages_unmap;
	struct page **pages;
	int ret;

	npages = size / PAGE_SIZE;
	pages = kvmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	for (i = 0; i < npages; i++)
		pages[i] = vmalloc_to_page(addr + i * PAGE_SIZE);

	vma->vm_page_prot = prot;
	npages_unmap = npages;
	ret = vm_insert_pages(vma, vma->vm_start, pages, &npages_unmap);
	if (ret)
		pr_err("%lu of %lu failed to be mapped.\n", npages_unmap, npages);

	kvfree(pages);
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
	size_t size;
	void *addr;
	int ret;
	pid_t pid;

	size = vma->vm_end - vma->vm_start;

	if (size == 0 || (size % PAGE_SIZE) != 0) {
		pr_err("invalid size %zu\n", size);
		return -EINVAL;
	}

	addr = vmalloc_user(size);
	if (!addr) {
		pr_err("failed to allocate %zu bytes\n", size);
		return -ENOMEM;
	}

	memset(addr, 0, size);

	if (g_prot == PROT_CACHEABLE) {
		ret = remap_vmalloc_range(vma, addr, 0);
	} else {
		pgprot_t prot = get_pgprot(g_prot);
		ret = map_vmalloc_range(vma, addr, size, prot);
	}

	if (ret) {
		pr_err("failed to map memory: %d\n", ret);
		vfree(addr);
		return ret;
	}

	region = kzalloc(sizeof(*region), GFP_KERNEL);
	if (!region) {
		pr_err("failed to allocate region\n");
		vfree(addr);
		return -ENOMEM;
	}

	pid = task_pid_nr(current->group_leader);
	region->pid = pid;
	region->user_va = vma->vm_start;
	region->size = size;
	region->kernel_va = addr;
	INIT_LIST_HEAD(&region->list);

	mutex_lock(&dev_lock);
	list_add(&region->list, &state->regions);
	mutex_unlock(&dev_lock);

	atomic_inc(&mapped_count);

	vma->vm_private_data = region;
	vma->vm_ops = &memdev_vm_ops;

	pr_info("mapped region pid=%d va=%#lx size=%#zx\n", pid, region->user_va, region->size);
	return 0;
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

	pr_info("initialized, device at %s (attr=%s, mapped_count=%d)\n",
		DEVICE_PATH, memdev_attr, atomic_read(&mapped_count));
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

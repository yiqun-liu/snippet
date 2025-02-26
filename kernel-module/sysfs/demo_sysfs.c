/*
 * A demo of sysfs.
 * See /sys/devices/DEMO_ROOT_DEV_NAME/DEMO_DEV_NAME (/sys/devices/demo-root/demo-sysfs).
 *
 * TODO elaborate on interface function signature and call chain
 *
 * references:
 *   - net/core/net-sysfs.c
 *   - drivers/acpi/bgrt.c
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

/* struct entity defined here */
#include "demo.h"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define DEMO_NUM_ATTR_GROUPS	1

static const char *root_dev_name = DEMO_ROOT_DEV_NAME;
static struct device *root_dev;
struct entity_device *entity_dev;

/* any entity associated with the device */
struct entity_device {
	/* data */
	struct entity entity;

	/* attribute group length: add an extra 1 for NULL terminator */
	const struct attribute_group *attr_groups[DEMO_NUM_ATTR_GROUPS + 1];
	struct device dev;
};

/*
 * Macros to set up a device-owned text-based attribute.
 * Define and initialize dev_attr_##name
 * See DEVICE_ATTR_RO (0444 permission), DEVICE_ATTR_RW (0666), DEVICE_ATTR_ADMIN_RO (0400).
 */
/* show one field of a device-related struct */
#define ENTITY_SHOW(field, formatter)							\
static ssize_t field##_show(struct device *dev, struct device_attribute *attr,		\
			    char *buf)							\
{											\
	struct entity_device *e_dev = container_of(dev, struct entity_device, dev);	\
	return sysfs_emit(buf, formatter, e_dev->entity.field);				\
}
/* define a read-only attribute fora device-related struct */
#define ENTITY_SHOW_RO(field, formatter)						\
ENTITY_SHOW(field, formatter);								\
static DEVICE_ATTR_RO(field)

/* use macros to define attribute instances */
ENTITY_SHOW_RO(magic, "%#lx");
ENTITY_SHOW_RO(version, "%s");

/* NULL-terminated attribute list: create groups to organize them in different directories */
static struct attribute *root_attrs[] __ro_after_init = {
	&dev_attr_magic.attr,
	&dev_attr_version.attr,
	NULL,
};

/*
 * Define a binary attribute.
 *
 * Binary attribute can be defined in a variety of ways. The wrapper we used here is significantly
 * different from its counterpart defined for device attribute. The attribute bin_attr_##name is
 * defined first, and the data reference pointer is set later, before the sysfs file is made online.
 *
 * The permission control is pretty much like device attributes.
 */
BIN_ATTR_SIMPLE_RO(data);
static void __init init_bin_attr_data(struct entity_device *entity_dev)
{
	bin_attr_data.private = &entity_dev->entity.data;
	bin_attr_data.size = sizeof(entity_dev->entity.data);
}
static struct bin_attribute *root_bin_attrs[] __ro_after_init = {
	&bin_attr_data,
	NULL,
};

/* grouping attributes into directory hierarchy */
static struct attribute_group root_attr_group __ro_after_init = {
	/* root level */
	.name = NULL,
	.attrs = root_attrs,
	.bin_attrs = root_bin_attrs,
};

static void release_entity_device(struct device *dev)
{
	struct entity_device *entity = container_of(dev, struct entity_device, dev);
	kfree(entity);
	pr_info("%s: done.\n", __func__);
}

static int create_and_online_entity_device(void)
{
	int ret;

	entity_dev = (struct entity_device*)kzalloc(sizeof(struct entity_device), GFP_KERNEL);
	if (entity_dev == NULL) {
		pr_err("%s: memory allocation failed for entity.\n", __func__);
		return -ENOMEM;
	}

	memcpy(&entity_dev->entity, &entity_template, sizeof(struct entity));
	init_bin_attr_data(entity_dev);
	entity_dev->attr_groups[0] = &root_attr_group;
	entity_dev->attr_groups[1] = NULL;
	device_initialize(&entity_dev->dev);
	entity_dev->dev.release = release_entity_device;
	entity_dev->dev.groups = entity_dev->attr_groups;
	entity_dev->dev.parent = root_dev;
	ret = dev_set_name(&entity_dev->dev, DEMO_DEV_NAME);
	if (ret) {
		pr_err("%s: failed to initialize device name, ret=%d.\n", __func__, ret);
		kfree(entity_dev);
		entity_dev = NULL;
		return ret;
	}

	ret = device_add(&entity_dev->dev);
	if (ret) {
		pr_err("%s: failed to add device %s, ret=%d.\n", __func__, DEMO_DEV_NAME, ret);
		kfree(entity_dev);
		entity_dev = NULL;
		return ret;
	}

	pr_info("%s: done.\n", __func__);
	return 0;
}
static void offline_entity_device(void)
{
	device_del(&entity_dev->dev);
	put_device(&entity_dev->dev);
	pr_info("%s: done\n", __func__);
}

static int __init demo_sysfs_init(void)
{
	int ret;

	root_dev = root_device_register(root_dev_name);
	if (IS_ERR_OR_NULL(root_dev)) {
		pr_err("%s: root device register failed.\n", __func__);
		return -EPERM;
	}
	ret = create_and_online_entity_device();
	if (ret) {
		root_device_unregister(root_dev);
		root_dev = NULL;
		return ret;
	}
	pr_info("%s: done.\n", __func__);
	return 0;
}

static void __exit demo_sysfs_exit(void)
{
	offline_entity_device();
	root_device_unregister(root_dev);
	root_dev = NULL;
	pr_info("%s: done.\n", __func__);
}

module_init(demo_sysfs_init);
module_exit(demo_sysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A demo of Linux sysfs");

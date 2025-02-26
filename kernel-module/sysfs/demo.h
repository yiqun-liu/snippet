/*
 * Common utilities shared by the kernel module and user space checker.
 */
#ifndef DEMO_H
#define DEMO_H

#define DEMO_ROOT_DEV_NAME	"demo-root"
#define DEMO_DEV_NAME		"demo-sysfs"

#define DEMO_VERSION_STR_LEN	32
#define DEMO_VERSION		"2025.02.26"
#define DEMO_MAGIC		0x31415926UL
#define DEMO_DATA_SIZE		8

struct entity {
	unsigned long magic;
	char version[DEMO_VERSION_STR_LEN];
	char data[DEMO_DATA_SIZE];
};

const struct entity entity_template = {
	.magic = DEMO_MAGIC,
	.version = DEMO_VERSION,
	.data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
};

#endif

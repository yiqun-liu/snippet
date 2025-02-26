/*
 * A C sysfs file reader
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "demo.h"

#define SYSFS_DIR	"/sys/devices/" DEMO_ROOT_DEV_NAME "/" DEMO_DEV_NAME

static int read_entity_ascii(struct entity *entity)
{
	FILE *fp;
	fp = fopen(SYSFS_DIR "/magic", "r");
	if (fp == NULL) {
		perror("fopen magic failed");
		return -1;
	}
	if (fscanf(fp, "%lx", &entity->magic) != 1) {
		fprintf(stderr, "magic scanf failed\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	fp = fopen(SYSFS_DIR "/version", "r");
	if (fp == NULL) {
		perror("fopen version failed");
		return -1;
	}
	if (fscanf(fp, "%s", entity->version) != 1) {
		fprintf(stderr, "version scanf failed\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}
static int read_entity_binary(struct entity *entity)
{
	int fd;
	ssize_t num_bytes;

	fd = open(SYSFS_DIR "/data", O_RDONLY);
	if (fd == -1) {
		perror("open data failed");
		return -1;
	}
	num_bytes = read(fd, entity->data, sizeof(entity->data));
	if (num_bytes == -1) {
		perror("parse data failed");
		close(fd);
		return -1;
	} else if (num_bytes != sizeof(entity->data)) {
		fprintf(stderr, "imcomplete data read: expect %zd, got %zd\n",
			sizeof(entity->data), num_bytes);
		close(fd);
		return -1;
	}
	return 0;
}

static int read_entity(struct entity *entity)
{
	int ret;
	ret = read_entity_ascii(entity);
	if (ret)
		return ret;
	ret = read_entity_binary(entity);
	return 0;
}

int main(void)
{
	int ret;
	struct entity entity;
	memset(&entity, 0, sizeof(entity));

	ret = read_entity(&entity);
	if (ret != 0) {
		fprintf(stderr, "failed to read entity from sysfs.\n");
		return ret;
	}

	ret = memcmp(&entity, &entity_template, sizeof(struct entity));
	if (ret == 0)
		printf("entity read from sysfs matched with its template\n");
	else
		printf("entity read from sysfs not matched with its template\n");

	return ret;
}

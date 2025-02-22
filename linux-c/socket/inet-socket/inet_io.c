#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "inet_io.h"

const char *payload_type_str[] = {
	[DEMO_STRING_PAYLOAD] = "string",
	[DEMO_UINT64_PAYLOAD] = "unsigned number",
	[DEMO_INT_ARRAY_PAYLOAD] = "integer array",
	[DEMO_STRUCT_PAYLOAD] = "user-defined struct",
};

void print_payload(enum payload_type type, size_t length, void *value)
{
	printf("packet: type=%d(%s), length=%zd, value=", type, payload_type_str[type], length);
	if (type == DEMO_STRING_PAYLOAD) {
		printf("%s\n", (const char*)value);
	} else if (type == DEMO_UINT64_PAYLOAD) {
		printf("%#lx\n", *(uint64_t*)value);
	} else if (type == DEMO_INT_ARRAY_PAYLOAD) {
		unsigned long count = length / sizeof(int);
		int *cursor = (int*)value;

		printf("[");
		while (count--)
			printf(" %d", *cursor++);
		printf(" ]\n");
	} else if (type == DEMO_STRUCT_PAYLOAD) {
		struct demo_struct_payload *payload = (struct demo_struct_payload *)value;
		printf("{str_info=%s, magic=%#lx}\n", payload->str_info, payload->magic);
	} else {
		printf("not-printable\n");
	}
}

int send_packet(int fd, enum payload_type type, size_t length, void *value)
{
	struct demo_metadata metadata = {
		.type = type,
		.length = length
	};
	ssize_t num_bytes;

	num_bytes = write(fd, &metadata, sizeof(metadata));
	if (num_bytes == -1) {
		fprintf(stderr, "%s: failed to write metadata to connection %d: %s.\n", __func__,
			fd, strerror(errno));
		return -1;
	} else if (num_bytes < 0 || (size_t)num_bytes != sizeof(metadata)) {
		fprintf(stderr, "%s: failed to write full metadata, wrote %zd bytes, "
			"expect %zd bytes.\n", __func__, num_bytes, sizeof(struct demo_metadata));
		return -1;
	}

	if (length == 0)
		return 0;

	num_bytes = write(fd, value, length);
	if (num_bytes == -1) {
		fprintf(stderr, "%s: failed to write payload to connection %d: %s.\n", __func__,
			fd, strerror(errno));
		return -1;
	} else if (num_bytes < 0 || (size_t)num_bytes != length) {
		fprintf(stderr, "%s: failed to write full payload, wrote %zd bytes, "
			"expect %zd bytes.\n", __func__, num_bytes, length);
		return -1;
	}

	return 0;
}

struct demo_packet *recv_payload(int fd)
{
	ssize_t num_bytes;
	struct demo_packet *packet;

	packet = (struct demo_packet*)malloc(sizeof(struct demo_packet));
	if (packet == NULL) {
		fprintf(stderr, "%s: failed to allocate packet memory.\n", __func__);
		return NULL;
	}

	memset(&packet->metadata, 0, sizeof(struct demo_metadata));

	num_bytes = read(fd, &packet->metadata, sizeof(struct demo_metadata));
	if (num_bytes == -1) {
		fprintf(stderr, "%s: failed to read metadata from connection %d: %s.\n",
			__func__, fd, strerror(errno));
		goto err_free_packet;
	} else if (num_bytes < 0 || (size_t)num_bytes != sizeof(struct demo_metadata)) {
		fprintf(stderr, "%s: failed to read full metadata from connection %d.\n",
			__func__, fd);
		goto err_free_packet;
	}

	if (packet->metadata.length == 0) {
		packet->payload = NULL;
		return packet;
	}

	packet->payload = malloc(packet->metadata.length);
	if (packet->payload == NULL) {
		fprintf(stderr, "%s: failed to allocate payload memory.\n", __func__);
		goto err_free_packet;
	}

	num_bytes = read(fd, packet->payload, packet->metadata.length);
	if (num_bytes == -1) {
		fprintf(stderr, "%s: failed to read payload from connection %d: %s.\n",
			__func__, fd, strerror(errno));
		goto err_free_packet;
	} else if (num_bytes < 0 || (size_t)num_bytes != packet->metadata.length) {
		fprintf(stderr, "%s: failed to read full payload from connection %d.\n",
			__func__, fd);
		goto err_free_packet;
	}

	return packet;

err_free_packet:
	free(packet);
	return NULL;
}

void free_packet(struct demo_packet *packet)
{
	if (packet->payload)
		free(packet->payload);
	free(packet);
}

/*
 * Stream-based payload send / recv & serialization / deserialization utilities. The TLV (type,
 * length, value) representation is used.
 */
#ifndef INET_IO_H
#define INET_IO_H

#include <stddef.h>
#include <stdint.h>

enum payload_type {
	DEMO_STRING_PAYLOAD,
	DEMO_UINT64_PAYLOAD,
	DEMO_INT_ARRAY_PAYLOAD,
	DEMO_STRUCT_PAYLOAD
};

#define DEMO_STR_INFO_LEN 7
struct demo_struct_payload {
	char str_info[DEMO_STR_INFO_LEN];
	unsigned long magic;
} __attribute__((aligned(8)));

struct demo_metadata {
	enum payload_type type;
	/* number of bytes */
	size_t length;
} __attribute__((aligned(8)));

struct demo_packet {
	struct demo_metadata metadata;
	void *payload;
};

/* debug */
void print_payload(enum payload_type type, size_t length, void *value);
/* Send a packet through the socket file descriptor. Return 0 on success. */
int send_packet(int fd, enum payload_type type, size_t length, void *value);
/* Read a packet from the socket file descriptor. Return NULL on failure. Return a dynamically
 * allocated struct on success. The caller is in charge of calling free_packet to free the memory. */
struct demo_packet *recv_payload(int fd);
void free_packet(struct demo_packet *packet);

#endif

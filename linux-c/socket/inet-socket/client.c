/* Connect to a server, write two TLV objects to the stream and read two TLV objects afterwards. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "inet_socket.h"
#include "inet_io.h"
#include "inet_socket_demo.h"

static int run_client_lite(const char *server_ip, uint16_t port)
{
	int ret, sock_fd;
	struct sockaddr_in server_addr;
	char server_desc[64];
	const char *string_payload = "hello world";
	uint64_t uint64_payload = 0x20250222 ;
	struct demo_packet *packet;

	ret = fill_sockaddr_in(&server_addr, server_ip, port);
	if (ret) {
		fprintf(stderr, "%s: failed to fill sock_addr, ret=%d.\n", __func__, ret);
		return ret;
	}

	snprintf_addr(server_desc, sizeof(server_desc), &server_addr);
	printf("client: prepared to connect to server: %s.\n", server_desc);
	sock_fd = inet4_connect(&server_addr, SOCK_STREAM);
	if (sock_fd <= 0) {
		fprintf(stderr, "%s: failed to connect, ret=%d.\n", __func__, ret);
		return ret;
	}
	printf("client: connection succeeded. prepared to send packets:\n");
	printf("[client]****************************************\n");
	print_payload(DEMO_STRING_PAYLOAD, strlen(string_payload) + 1, (void*)string_payload);
	print_payload(DEMO_UINT64_PAYLOAD, sizeof(uint64_payload), (void*)&uint64_payload);
	printf("[client]****************************************\n");

	ret = send_packet(sock_fd, DEMO_STRING_PAYLOAD, strlen(string_payload) + 1,
		   (void*)string_payload);
	if (ret) {
		fprintf(stderr, "%s: failed to send string payload, ret=%d.\n", __func__, ret);
		goto out_close;
	}

	ret = send_packet(sock_fd, DEMO_UINT64_PAYLOAD, sizeof(uint64_payload),
		   (void*)&uint64_payload);
	if (ret) {
		fprintf(stderr, "%s: failed to send uint64_t payload, ret=%d.\n", __func__, ret);
		goto out_close;
	}
	printf("client: all packets sent out, start to dump response received.\n\n");

	packet = recv_payload(sock_fd);
	if (packet == NULL) {
		fprintf(stderr, "%s: failed to recv 1st payload.\n", __func__);
		goto out_close;
	}
	print_payload(packet->metadata.type, packet->metadata.length, packet->payload);
	free_packet(packet);

	packet = recv_payload(sock_fd);
	if (packet == NULL) {
		fprintf(stderr, "%s: failed to recv 2nd payload.\n", __func__);
		goto out_close;
	}
	print_payload(packet->metadata.type, packet->metadata.length, packet->payload);
	free_packet(packet);

	printf("client: packets received expectedly.\n");
	printf("client: terminate normally.\n");

out_close:
	close(sock_fd);
	return ret;
}

int main()
{
	run_client_lite(SERVER_IP_0, SERVER_PORT_0);
	return 0;
}

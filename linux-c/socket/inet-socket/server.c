/* Establiash a connection, read two TLV objects from the stream and write two TLV objects in
 * response. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include "inet_socket.h"
#include "inet_io.h"
#include "inet_socket_demo.h"

#define MAX_PENDING_CONNECTIONS 5

static int run_server_lite(const char *server_ip, uint16_t port)
{
	int ret, sock_fd, conn_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addrlen;
	char server_desc[64], client_desc[64];
	int array_payload[] = {INT_MIN, 0, INT_MAX};
	struct demo_struct_payload struct_payload = {
		.str_info = "struct",
		.magic = 0xABCDEF0123456789UL
	};
	struct demo_packet *packet;

	ret = fill_sockaddr_in(&server_addr, server_ip, port);
	if (ret) {
		fprintf(stderr, "%s: failed to fill sock_addr, ret=%d.\n", __func__, ret);
		return ret;
	}

	snprintf_addr_name(server_desc, sizeof(server_desc), &server_addr);
	printf("server: prepared to listen on %s.\n", server_desc);
	sock_fd = inet4_listen(&server_addr, SOCK_STREAM, MAX_PENDING_CONNECTIONS);
	if (sock_fd <= 0) {
		fprintf(stderr, "%s: failed to listen, ret=%d.\n", __func__, ret);
		return ret;
	}

	ret = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addrlen);
	if (ret == -1) {
		fprintf(stderr, "%s: failed to accept connection from socket: %s.\n",
			__func__, strerror(errno));
		goto out_close_listen;
	}
	conn_fd = ret;

	snprintf_addr_name(client_desc, sizeof(client_desc), &client_addr);
	printf("server: connection channel ready: the peer is %s.\n", client_desc);
	packet = recv_payload(conn_fd);
	if (packet == NULL) {
		fprintf(stderr, "%s: failed to recv 1st payload.\n", __func__);
		goto out_close_conn;
	}
	print_payload(packet->metadata.type, packet->metadata.length, packet->payload);
	free_packet(packet);

	packet = recv_payload(conn_fd);
	if (packet == NULL) {
		fprintf(stderr, "%s: failed to recv 2nd payload.\n", __func__);
		goto out_close_conn;
	}
	print_payload(packet->metadata.type, packet->metadata.length, packet->payload);
	free_packet(packet);

	printf("server: all packets received, start to send response packets.\n\n");
	printf("[server]****************************************\n");
	print_payload(DEMO_INT_ARRAY_PAYLOAD, sizeof(array_payload), (void*)array_payload);
	print_payload(DEMO_STRUCT_PAYLOAD, sizeof(struct_payload), (void*)&struct_payload);
	printf("[server]****************************************\n");

	ret = send_packet(conn_fd, DEMO_INT_ARRAY_PAYLOAD, sizeof(array_payload), (void*)array_payload);
	if (ret) {
		fprintf(stderr, "%s: failed to send array payload, ret=%d.\n", __func__, ret);
		goto out_close_conn;
	}

	ret = send_packet(conn_fd, DEMO_STRUCT_PAYLOAD, sizeof(struct_payload), (void*)&struct_payload);
	if (ret) {
		fprintf(stderr, "%s: failed to send struct payload, ret=%d.\n", __func__, ret);
		goto out_close_conn;
	}

	printf("server: packets sent out expectedly.\n");
	printf("server: terminate normally.\n");

out_close_conn:
	close(conn_fd);
out_close_listen:
	close(sock_fd);
	return ret;
}

int main()
{
	run_server_lite(SERVER_IP_0, SERVER_PORT_0);
	return 0;
}

/* A demo of UNIX socket */
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

/* server-client global definitions */
#include "unix_socket_demo.h"

#define MAX_PENDING 5

/* passively connect to client, wait for an req and reply with a ack */
static int run_stream_server()
{
	struct sockaddr_un server_addr, client_addr;
	socklen_t client_addrlen;
	int ret, sock_fd, conn_fd;
	char data_buffer[DATA_BUFFER_SIZE];
	ssize_t num_bytes;

	/* create a socket: unix-domain, stream type, default protocol of unix-domain family */
	ret = socket(AF_UNIX, SOCK_STREAM, 0);
	if (ret == -1) {
		fprintf(stderr, "server failed to create socket: %s\n", strerror(errno));
		return ret;
	}
	sock_fd = ret;

	/* bind to a well-known address and create socket file */
	set_unix_socket_path(&server_addr, SERVER_STREAM_SOCKET_PATH);
	ret = bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un));
	if (ret == -1) {
		fprintf(stderr, "server failed to bind socket fd %d to addr %s: %s\n", sock_fd,
			SERVER_STREAM_SOCKET_PATH, strerror(errno));
		goto out_close;
	}

	/* start waiting for connections */
	ret = listen(sock_fd, MAX_PENDING);
	if (ret == -1) {
		fprintf(stderr, "server failed to listen to socket, sock_fd=%d, backlog=%d: %s.\n",
			sock_fd, MAX_PENDING, strerror(errno));
		goto out_unlink;
	}

	printf("server(pid=%d): starts listening on stream socket %s.\n", getpid(),
		SERVER_STREAM_SOCKET_PATH);

	/* handle exactly one pending connection (in reality, this can be put into a loop) */
	client_addrlen = sizeof(client_addr);
	ret = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addrlen);
	if (ret == -1) {
		fprintf(stderr, "failed to accept a socket: %s.\n", strerror(errno));
		goto out_unlink;
	}
	conn_fd = ret;

	/* when the client socket has no binding, its address is empty */
	printf("server(pid=%d): connected with client(sock_addr=\"%s\", addr_len=%u)\n",
		getpid(), client_addr.sun_path, client_addrlen);

	/* read from client and check if it's a "REQ" */
	num_bytes = read(conn_fd, data_buffer, DATA_BUFFER_SIZE);
	if (num_bytes == -1) {
		fprintf(stderr, "server(pid=%d): failed to read from connection %d: %s.\n",
			getpid(), conn_fd, strerror(errno));
		ret = -1;
		goto out_unlink;
	} else if (num_bytes == 0) {
		fprintf(stderr, "server(pid=%d): got empty data.\n", getpid());
		ret = -1;
		goto out_unlink;
	}

	printf("server(pid=%d): received data (%zd bytes) \"%s\".\n", getpid(), num_bytes,
		data_buffer);
	if (strncmp(CLIENT_REQ_MSG, data_buffer, strlen(CLIENT_REQ_MSG) + 1) != 0) {
		fprintf(stderr, "server(pid=%d): unexpected data received.\n", getpid());
		ret = -1;
		goto out_unlink;
	}

	/* write to client the "ACK" */
	num_bytes = write(conn_fd, SERVER_ACK_MSG, strlen(SERVER_ACK_MSG) + 1);
	printf("server(pid=%d): wrote %zd bytes -- \"%s\".\n", getpid(), num_bytes, SERVER_ACK_MSG);
	if (num_bytes == -1) {
		fprintf(stderr, "server(pid=%d): failed to write to connection %d: %s.\n",
			getpid(), conn_fd, strerror(errno));
		ret = -1;
		goto out_unlink;
	} else if (num_bytes != strlen(SERVER_ACK_MSG) + 1) {
		fprintf(stderr, "server -> client: failed to write full data, wrote %zd bytes, "
			"expect %zd bytes.\n", num_bytes, strlen(SERVER_ACK_MSG) + 1);
		ret = -1;
		goto out_unlink;
	}
	printf("server(pid=%d): %s terminated exepctedly.\n", getpid(), __func__);
	ret = 0;

out_unlink:
	if (unlink(SERVER_STREAM_SOCKET_PATH) == -1) {
		fprintf(stderr, "server failed to remove socket file: %s\n", strerror(errno));
		ret = ret ? ret : -1;
	}
out_close:
	close(sock_fd);
	return ret;
}

static int run_datagram_server()
{
	struct sockaddr_un server_addr, client_addr;
	socklen_t client_addrlen;
	int ret, sock_fd;
	char data_buffer[DATA_BUFFER_SIZE];
	ssize_t num_bytes;

	/* create a socket: unix-domain, datagram type, default protocol of unix-domain family */
	ret = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (ret == -1) {
		fprintf(stderr, "server failed to create socket: %s\n", strerror(errno));
		return ret;
	}
	sock_fd = ret;

	/* bind to a well-known address and create socket file */
	set_unix_socket_path(&server_addr, SERVER_DGRAM_SOCKET_PATH);
	ret = bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un));
	if (ret == -1) {
		fprintf(stderr, "server failed to bind socket fd %d to addr %s: %s\n", sock_fd,
			SERVER_DGRAM_SOCKET_PATH, strerror(errno));
		goto out_close;
	}
	printf("server(pid=%d): datagram socket %s setup.\n", getpid(),
		SERVER_DGRAM_SOCKET_PATH);

	/* handle exactly one pending datagram (in reality, this can be put into a loop) */
	/* read from client and check if it's a "REQ" */
	client_addrlen = sizeof(client_addr);
	num_bytes = recvfrom(sock_fd, data_buffer, DATA_BUFFER_SIZE, 0,
		      (struct sockaddr*)&client_addr, &client_addrlen);
	if (num_bytes == -1) {
		fprintf(stderr, "server(pid=%d): recvfrom() sock_fd %d failed : %s.\n",
			getpid(), sock_fd, strerror(errno));
		ret = -1;
		goto out_unlink;
	} else if (num_bytes == 0) {
		fprintf(stderr, "server(pid=%d): got empty data.\n", getpid());
		ret = -1;
		goto out_unlink;
	}

	printf("server(pid=%d): received datagram (%zd bytes) \"%s\" from peer \"%s\".\n", getpid(),
		num_bytes, data_buffer, client_addr.sun_path);
	if (strncmp(CLIENT_REQ_MSG, data_buffer, strlen(CLIENT_REQ_MSG) + 1) != 0) {
		fprintf(stderr, "server(pid=%d): unexpected data received.\n", getpid());
		ret = -1;
		goto out_unlink;
	}

	/* write to client the "ACK" */
	num_bytes = sendto(sock_fd, SERVER_ACK_MSG, strlen(SERVER_ACK_MSG) + 1,
		0, (struct sockaddr*)&client_addr, client_addrlen);
	printf("server(pid=%d): send a %zd bytes datagram -- \"%s\".\n", getpid(),
		strlen(SERVER_ACK_MSG) + 1, SERVER_ACK_MSG);
	if (num_bytes == -1) {
		fprintf(stderr, "server(pid=%d): failed to write to datagram socket %d: %s.\n",
			getpid(), sock_fd, strerror(errno));
		ret = -1;
		goto out_unlink;
	} else if (num_bytes != strlen(SERVER_ACK_MSG) + 1) {
		fprintf(stderr, "server -> client: failed to send full data, wrote %zd bytes, "
			"expect %zd bytes.\n", num_bytes, strlen(SERVER_ACK_MSG) + 1);
		ret = -1;
		goto out_unlink;
	}
	printf("server(pid=%d): %s terminated exepctedly.\n", getpid(), __func__);
	ret = 0;

out_unlink:
	if (unlink(SERVER_DGRAM_SOCKET_PATH) == -1) {
		fprintf(stderr, "server failed to remove socket file: %s\n", strerror(errno));
		ret = ret ? ret : -1;
	}
out_close:
	close(sock_fd);
	return ret;
}

int main()
{
	int ret;

	ret = run_stream_server();
	if (ret) {
		fprintf(stderr, "failed to run stream server demo, ret=%d.\n", ret);
		return ret;
	}

	printf("server---server---server---server---server---server---\n");

	ret  = run_datagram_server();
	if (ret) {
		fprintf(stderr, "failed to run datagram server demo, ret=%d.\n", ret);
		return ret;
	}

	return 0;
}

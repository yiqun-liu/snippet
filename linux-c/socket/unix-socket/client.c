/* A demo of UNIX socket */
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

/* server-client global definitions */
#include "unix_socket_demo.h"

#define MAX_PENDING 5

/* actively connect to server, send ack and wait for an ack */
static int run_stream_client()
{
	struct sockaddr_un server_addr;
	int ret, sock_fd;
	char data_buffer[DATA_BUFFER_SIZE];
	ssize_t num_bytes;

	/* create a socket: unix-domain, stream type, default protocol of unix-domain family */
	ret = socket(AF_UNIX, SOCK_STREAM, 0);
	if (ret == -1) {
		fprintf(stderr, "client failed to create socket: %s\n", strerror(errno));
		return ret;
	}
	sock_fd = ret;

	/* Binding of socket to a address is optional: Since there is a connection between server
	 * and client, communication can be done without a client address. This it Not the case for
	 * datagram. */

	/* connect to a well-known address and create socket file */
	set_unix_socket_path(&server_addr, SERVER_STREAM_SOCKET_PATH);
	ret = connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un));
	if (ret == -1) {
		fprintf(stderr, "client failed to connect to socket, sock_fd=%d, server_addr=%s: "
			"%s.\n", sock_fd, server_addr.sun_path, strerror(errno));
		goto out_close;
	}

	printf("client(pid=%d): connected to server(sock_addr=%s)\n",
		getpid(), server_addr.sun_path);

	/* unlike the server, the client performs IO directly through its inital socket */
	/* write to server its "request" */
	num_bytes = write(sock_fd, CLIENT_REQ_MSG, strlen(CLIENT_REQ_MSG) + 1);
	printf("client(pid=%d): wrote %zd bytes -- \"%s\".\n", getpid(), num_bytes, CLIENT_REQ_MSG);
	if (num_bytes == -1) {
		fprintf(stderr, "client(pid=%d): failed to write to connection %d: %s.\n",
			getpid(), sock_fd, strerror(errno));
		ret = -1;
		goto out_close;
	} else if (num_bytes != strlen(CLIENT_REQ_MSG) + 1) {
		fprintf(stderr, "client -> server: failed to write full data, wrote %zd bytes, "
			"expect %zd bytes.\n", num_bytes, strlen(SERVER_ACK_MSG) + 1);
		ret = -1;
		goto out_close;
	}

	/* read from server and check if it's an "ACK" response */
	num_bytes = read(sock_fd, data_buffer, DATA_BUFFER_SIZE);
	if (num_bytes == -1) {
		fprintf(stderr, "client(pid=%d): failed to read from connection %d: %s.\n",
			getpid(), sock_fd, strerror(errno));
		ret = -1;
		goto out_close;
	} else if (num_bytes == 0) {
		fprintf(stderr, "client(pid=%d): got empty data.\n", getpid());
		ret = -1;
		goto out_close;
	}

	printf("client(pid=%d): received data (%zd bytes) \"%s\".\n", getpid(), num_bytes,
		data_buffer);
	if (strncmp(SERVER_ACK_MSG, data_buffer, strlen(SERVER_ACK_MSG) + 1) != 0) {
		fprintf(stderr, "client(pid=%d): unexpected data received.\n", getpid());
		ret = -1;
		goto out_close;
	}

	printf("client(pid=%d): %s terminated exepctedly.\n", getpid(), __func__);
	ret = 0;

out_close:
	close(sock_fd);
	return ret;
}

static int run_datagram_client()
{
	struct sockaddr_un server_addr, client_addr;
	int ret, sock_fd;
	char data_buffer[DATA_BUFFER_SIZE];
	ssize_t num_bytes;

	/* create a socket: unix-domain, datagram type, default protocol of unix-domain family */
	ret = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (ret == -1) {
		fprintf(stderr, "client failed to create socket: %s\n", strerror(errno));
		return ret;
	}
	sock_fd = ret;

	/* binding of socket to a address is mandatory to receive any datagram */
	set_unix_socket_path(&client_addr, CLIENT_DGRAM_SOCKET_PATH);
	ret = bind(sock_fd, (struct sockaddr*)&client_addr, sizeof(struct sockaddr_un));
	if (ret == -1) {
		fprintf(stderr, "client failed to bind socket fd %d to addr %s: %s\n", sock_fd,
			CLIENT_DGRAM_SOCKET_PATH, strerror(errno));
		goto out_close;
	}

	printf("client(pid=%d): datagram socket %s setup.\n", getpid(),
		CLIENT_DGRAM_SOCKET_PATH);

	/* unlike the server, the client performs IO directly through its inital socket */
	/* write to server its "request" */
	set_unix_socket_path(&server_addr, SERVER_DGRAM_SOCKET_PATH);
	num_bytes = sendto(sock_fd, CLIENT_REQ_MSG, strlen(CLIENT_REQ_MSG) + 1,
		0, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un));
	printf("client(pid=%d): send a %zd bytes datagram -- \"%s\".\n", getpid(),
		strlen(CLIENT_REQ_MSG) + 1, CLIENT_REQ_MSG);
	if (num_bytes == -1) {
		fprintf(stderr, "client(pid=%d): failed to write to datagram socket %d: %s.\n",
			getpid(), sock_fd, strerror(errno));
		ret = -1;
		goto out_unlink;
	} else if (num_bytes != strlen(CLIENT_REQ_MSG) + 1) {
		fprintf(stderr, "client -> server: failed to send full data, wrote %zd bytes, "
			"expect %zd bytes.\n", num_bytes, strlen(SERVER_ACK_MSG) + 1);
		ret = -1;
		goto out_unlink;
	}

	/* read from server and check if it's an "ACK" response */
	num_bytes = recvfrom(sock_fd, data_buffer, DATA_BUFFER_SIZE, 0, NULL, NULL);
	if (num_bytes == -1) {
		fprintf(stderr, "client(pid=%d): recvfrom() sock_fd %d failed : %s.\n",
			getpid(), sock_fd, strerror(errno));
		ret = -1;
		goto out_unlink;
	} else if (num_bytes == 0) {
		fprintf(stderr, "client(pid=%d): got empty data.\n", getpid());
		ret = -1;
		goto out_unlink;
	}

	printf("client(pid=%d): received datagram (%zd bytes) \"%s\".\n", getpid(), num_bytes,
		data_buffer);
	if (strncmp(SERVER_ACK_MSG, data_buffer, strlen(SERVER_ACK_MSG) + 1) != 0) {
		fprintf(stderr, "client(pid=%d): unexpected data received.\n", getpid());
		ret = -1;
		goto out_unlink;
	}

	printf("client(pid=%d): %s terminated exepctedly.\n", getpid(), __func__);
	ret = 0;

out_unlink:
	if (unlink(CLIENT_DGRAM_SOCKET_PATH) == -1) {
		fprintf(stderr, "client failed to remove socket file: %s\n", strerror(errno));
		ret = ret ? ret : -1;
	}

out_close:
	close(sock_fd);
	return ret;
}

int main()
{
	int ret;

	ret = run_stream_client();
	if (ret) {
		fprintf(stderr, "failed to run stream client demo, ret=%d.\n", ret);
		return ret;
	}

	printf("***client***client***client***client***client***client\n");

	ret = run_datagram_client();
	if (ret) {
		fprintf(stderr, "failed to run stream client demo, ret=%d.\n", ret);
		return ret;
	}

	return 0;
}

/*
 * This header provides simple public constants and utilities shared by the client and server.
 */
#ifndef UNIX_SOCKET_DEMO_H
#define UNIX_SOCKET_DEMO_H

/* UNIX domain address type definitions */
#include <sys/un.h>
/* socket API definitions */
#include <sys/socket.h>

#define SERVER_STREAM_SOCKET_PATH "/tmp/unix_stream_socket"
#define SERVER_DGRAM_SOCKET_PATH "/tmp/unix_dgram_socket-server"
#define CLIENT_DGRAM_SOCKET_PATH "/tmp/unix_dgram_socket-client"
#define DATA_BUFFER_SIZE 64

#define CLIENT_REQ_MSG "CLIENT_REQ"
#define SERVER_ACK_MSG "SERVER_ACK"

static void set_unix_socket_path(struct sockaddr_un *sock_addr, const char *pathname)
{
	/* intialize the address structure, make sure the pathname string is NULL-terminated */
	memset(sock_addr, 0 ,sizeof(struct sockaddr_un));
	sock_addr->sun_family = AF_UNIX;
	strncpy((char*)sock_addr->sun_path, pathname, sizeof(sock_addr->sun_path) - 1);
}

#endif

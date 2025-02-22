/*
 * A wrapper of connection setup procedure. Not fully tested (datagram and the "modern" symbol-based
 * interfaces are not tested).
 */
#ifndef INET_SOCKET_H
#define INET_SOCKET_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* fill @sock_addr using @ip (dotted decimals for IPv4) string and @port number */
int fill_sockaddr_in(struct sockaddr_in *sock_addr, const char *ip, uint16_t port);
/* print IPv4 description string */
int snprintf_addr(char *buf, size_t buf_len, const struct sockaddr_in *sock_addr);
/* print IPv4 description string with more symbolized expression */
int snprintf_addr_name(char *buf, size_t buf_len, const struct sockaddr_in *sock_addr);

/* Create a socket and listen on the specified IPv4 addresses @sock_addr. Return socket file
 * descriptor on success, negative values on errors. The caller still need to call accept() on it to
 * intiate connection */
int inet4_listen(const struct sockaddr_in *sock_addr, int sock_type, int backlog);
/* Create a socket and connect to @sock_addr with @sock_type. Return socket file descriptor on
 * success and negative value on failure. */
int inet4_connect(const struct sockaddr_in *sock_addr, int sock_type);

/* The variants of wrapper above. Take in human-readable strings are parameters. (never tested) */
int inet4_listen_str(int sock_type, const char *service, int backlog);
int inet4_connect_str(int sock_type, const char *host, const char *service);

#endif

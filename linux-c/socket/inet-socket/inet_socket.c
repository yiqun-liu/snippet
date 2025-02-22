/*
 * Implementation of a few internet socket utilities.
 *
 * References
 *   * The Linux Programming Interface: Sockets: Internet Domains
 *   * manual pages
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
/* basic network struct conversion utilities */
#include <arpa/inet.h>
/* advanced address info utilities*/
#include <netdb.h>
/* socket configuration */
#include <sys/socket.h>

#include "inet_socket.h"

/* debug */
static const char *get_af_str(int address_family)
{
	static const char *family_str[AF_MAX + 1] = {
		[AF_UNSPEC] = "not specified",
		[AF_UNIX] = "UNIX",
		[AF_INET] = "IPv4",
		[AF_INET6] = "IPv6",
	};
	if (address_family >= AF_MAX)
		return "invalid address family";
	if (family_str[address_family])
		return family_str[address_family];
	return "unknown address family";
}
static const char *get_sock_type_str(int socket_type)
{
	if (socket_type == SOCK_STREAM)
		return "stream";
	if (socket_type == SOCK_DGRAM)
		return "data gram";
	return "unknown socket type";
}
static void print_addrinfo(const struct addrinfo *info)
{
	printf("addrinfo=%p\n", info);
	if (!info)
		return;

	printf("\tai_flags=%#x\n", info->ai_flags);
	printf("\tai_family=%#x (%s)\n", info->ai_family, get_af_str(info->ai_family));
	printf("\tai_socktype=%#x (%s)\n", info->ai_socktype, get_sock_type_str(info->ai_socktype));
	printf("\tai_protocol=%#x\n", info->ai_protocol);
	printf("\tai_addrlen=%#x\n", info->ai_addrlen);
	printf("\tai_conn=%p (%s)\n", info->ai_canonname, info->ai_canonname ? info->ai_canonname : "(NULL)");
	printf("\tai_next=%p\n", info->ai_next);
}

int fill_sockaddr_in(struct sockaddr_in *sock_addr, const char *ip, uint16_t port)
{
	int ret;
	memset(sock_addr, 0, sizeof(struct sockaddr_in));
	sock_addr->sin_family = AF_INET;
	sock_addr->sin_port = htons(port);
	if (sock_addr == NULL) {
		sock_addr->sin_addr.s_addr = INADDR_ANY;
	} else {
		/* dotted decimals presentation to network strucutre; return 1 on success */
		ret = inet_pton(AF_INET, ip, &sock_addr->sin_addr);
		if (ret == 0) {
			fprintf(stderr, "address not valid: '%s'\n", ip);
			return -1;
		} else if (ret != 1) {
			fprintf(stderr, "failed to convert IP string %s to sockaddr struct: ret=%d,"
				" '%s'\n", ip, ret, strerror(errno));
			return ret;
		}
	}
	return 0;
}

int snprintf_addr_name(char *buf, size_t buf_len, const struct sockaddr_in *sock_addr)
{
	int ret;
	/* the maximum length of host name and service name */
	char host[NI_MAXHOST], service[NI_MAXSERV];

	if (sock_addr->sin_family != AF_INET)
		return -EINVAL;

	ret = getnameinfo((struct sockaddr*)sock_addr, sizeof(struct sockaddr_in),
		host, NI_MAXHOST, service, NI_MAXSERV, 0);
	if (ret) {
		fprintf(stderr, "failed to get the name of sock_addr %p: ret=%d\n", sock_addr, ret);
		return -1;
	}
	return snprintf(buf, buf_len, "%s:%s", host, service);
}
int snprintf_addr(char *buf, size_t buf_len, const struct sockaddr_in *sock_addr)
{
	uint16_t port;
	char host[INET_ADDRSTRLEN];
	const char *str_ret;

	if (sock_addr->sin_family != AF_INET)
		return -EINVAL;

	port = ntohs(sock_addr->sin_port);
	str_ret = inet_ntop(AF_INET, &sock_addr->sin_addr, host, INET_ADDRSTRLEN);
	if (str_ret == NULL) {
		fprintf(stderr, "failed to get the host name form sock_addr %p: %s\n", sock_addr,
			strerror(errno));
		return -1;
	}
	return snprintf(buf, buf_len, "%s:%u", host, port);
}

int inet4_listen(const struct sockaddr_in *sock_addr, int sock_type, int backlog)
{
	int sock_fd, ret, socket_option_value;

	sock_fd = socket(AF_INET, sock_type, 0);
	if (sock_fd == -1) {
		fprintf(stderr, "failed to create socktet: %s.\n", strerror(errno));
		return -1;
	}

	socket_option_value = 1;
	ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option_value,
		sizeof(socket_option_value));
	if (ret == -1) {
		fprintf(stderr, "failed to configure socktet: %s.\n", strerror(errno));
		close(sock_fd);
		return -1;
	}

	ret = bind(sock_fd, (struct sockaddr*)sock_addr, sizeof(struct sockaddr_in));
	if (ret == -1) {
		fprintf(stderr, "failed to bind socktet: %s.\n", strerror(errno));
		close(sock_fd);
		return -1;
	}

	ret = listen(sock_fd, backlog);
	if (ret == -1) {
		fprintf(stderr, " failed to listen on socket, sock_fd=%d, backlog=%d: %s.\n",
			sock_fd, backlog, strerror(errno));
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

int inet4_connect(const struct sockaddr_in *sock_addr, int sock_type)
{
	int sock_fd, ret;

	sock_fd = socket(AF_INET, sock_type, 0);
	if (sock_fd == -1) {
		fprintf(stderr, "failed to create socktet: %s.\n", strerror(errno));
		return -1;
	}

	ret = connect(sock_fd, (struct sockaddr*)sock_addr, sizeof(struct sockaddr_in));
	if (ret == -1) {
		fprintf(stderr, "failed to connect to socktet: %s.\n", strerror(errno));
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

/*
 * 1. map port number to net byte order binaries
 * 2. create a socket
 * 3. bind to the port and listen
 */
int inet4_listen_str(int sock_type, const char *service, int backlog)
{
	int ret, sock_fd = -1;
	/* The result filter for {host, port} strings to net byte order data structure mapping.
	 * Prefix "ai" stands for "address information". A zero value or NULL pointer means "not
	 * care". */
	struct addrinfo hints = {
		/* look up IPv4 only */
		.ai_family = AF_INET,
		.ai_socktype = sock_type,
		/* request socket address which is suitable for binding  */
		.ai_flags = AI_PASSIVE,
	};
	struct addrinfo *results = NULL, *address;

	printf("Doing service (port) look up...\n");

	/* host=NULL: do not care about the IP address  */
	ret = getaddrinfo(NULL, service, &hints, &results);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo(port='%s') failed, ret=%d\n", service, ret);
		return ret;
	}
	if (results == NULL) {
		fprintf(stderr, "getaddrinfo(port='%s') failed: empty output\n", service);
		return -1;
	}

	for (address = results; address != NULL; address = address->ai_next) {
		int socket_option_value;
		print_addrinfo(address);

		/* not necessary to iterate all, loop it through only to see what address info may
		 * be returned */
		if (sock_fd != -1)
			continue;
		sock_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (sock_fd == -1) {
			fprintf(stderr, "failed to create socktet: %s. Try next\n",
				strerror(errno));
			continue;
		}
		socket_option_value = 1;
		ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option_value,
			sizeof(socket_option_value));
		if (ret == -1) {
			fprintf(stderr, "failed to configure socktet: %s. Try next\n",
				strerror(errno));
			close(sock_fd);
			sock_fd = -1;
		}

		ret = bind(sock_fd, address->ai_addr, address->ai_addrlen);
		if (ret == -1) {
			fprintf(stderr, "failed to bind socktet: %s. Try next\n",
				strerror(errno));
			close(sock_fd);
			sock_fd = -1;
		}

	}

	if (sock_fd == -1) {
		fprintf(stderr, "all alternatives tried: failed to create and bind socket.\n");
		ret = -1;
		goto out_free_results;
	}

	ret = listen(sock_fd, backlog);
	if (ret == -1) {
		fprintf(stderr, " failed to listen on socket, sock_fd=%d, backlog=%d: %s.\n",
			sock_fd, backlog, strerror(errno));
		goto out_close;
	}

	freeaddrinfo(results);
	return sock_fd;

out_close:
	close(sock_fd);
out_free_results:
	freeaddrinfo(results);

	return ret;
}

int inet4_connect_str(int sock_type, const char *host, const char *service)
{
	int ret, sock_fd = -1;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = sock_type,
	};
	struct addrinfo *results = NULL, *address;

	printf("Doing service (port) look up...\n");

	ret = getaddrinfo(host, service, &hints, &results);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo(host='%s', port='%s') failed, ret=%d\n", host, service,
			ret);
		return ret;
	}
	if (results == NULL) {
		fprintf(stderr, "getaddrinfo(port='%s') failed: empty output\n", service);
		return -1;
	}

	for (address = results; address != NULL; address = address->ai_next) {
		print_addrinfo(address);

		if (sock_fd != -1)
			continue;
		sock_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (sock_fd == -1) {
			fprintf(stderr, "failed to create socktet: %s. Try next\n",
				strerror(errno));
			continue;
		}

		ret = connect(sock_fd, address->ai_addr, address->ai_addrlen);
		if (ret == -1) {
			fprintf(stderr, "failed to connect to socktet: %s. Try next\n",
				strerror(errno));
			close(sock_fd);
			sock_fd = -1;
		}
	}

	if (sock_fd == -1) {
		fprintf(stderr, "all alternatives tried: failed to create and bind socket.\n");
		ret = -1;
		goto out_free_results;
	}

	freeaddrinfo(results);
	return sock_fd;

out_free_results:
	freeaddrinfo(results);

	return ret;
}

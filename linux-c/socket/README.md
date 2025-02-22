# The Socket API

communication domain
* naming
  * `AF_xxx`: address family
  * `PF_xxx`: protocol family
* `AF_UNIX`: communication between applications on the same host
  * alternative names: `AF_LOCAL`, `PF_UNIX`
  * address format: `sockaddr_un`, path name
* `AF_INET`: IPv4 domain
  * address format: `sockaddr_in`, IPv4 address + port number
* `AF_INET6`: IPv6 domain
  * address format: `sockaddr_in6`, IPv6 address + port number

socket type
* `SOCK_STREAM`: connection-oriented, reliable, birectional byte-stream
  * use TCP in `INET` & `INET6` domain
  * similar to a pair of pipes
* `SOCK_DGRAM`: connectionless, unrelable (out or order, duplicate and missing) datagram
  * reliability is not guaranteed, but may be partially implemented at some level (e.g. network)
    * therefore duplicate is possible
  * use UDP in `INET` & `INET6` domain
  * exception: in UNIX domain the delivery is in-order and reliable
  * always read one datagram; silently truncate the datagram when the buffer is not big enough
* `SOCK_RAW`: raw socket which allow applications to communicate directly on IP protocol

operations
* `socket()`: create a new socket and return the "sockfd" file descriptor
* `bind()`: bind a socket to an address (optional)
* stream socket:
  * `listen()`: wait for incoming connections (passive)
  * `connect()`: try to establish a connection (active)
  * `accept()`: accept an incoming connection request and create a new socket for that connection
    (passive)
  * `close()`: close the file descriptor as well as the connections
    * a socket is closed when all file descriptor get closed
  * `shutdown()`
  * stream socket follows a state machine: only certain operations are allowed in certain states
* datagram socket:
  * `connect()`: let kernel record the information of communication peer
    * motivation: use simpler IO API and make space for performance optimization
    * when "connected", the standard `write()` which does not have a peer address argument may be used
    * only datagrams from the specified peer may be `read()`
    * has no effect on the remote socket; there is still no real connection between the peers
* socket IO
  * general system calls: `read()`, `write()`
    * read from a socket closed on the other end: get end-of-file
    * write to a socket closed on the other end: receive `SIGPIPE` (ignored by default); syscall
      fails with `EPIPE`
  * socket-specific system calls: `send()`, `recv()`
    * the first three arguments (`fd`, `buffer`, `size` are kept the same as `read()` and `write()`)
    * datagram socket: `sendto()`, `recvfrom()` (when `flags == 0`, have same behavior as `write()`
      and `read()`)

pending connections
* when the client calls `connect()` and the server has not yet calls `accept()`, the connection is
  pending
* the `backlog` parameter of `listen()` controls the max number of pending connections allowed for
  a socket
  * `SOMAXCONN`: a constant exported by `sys/socket.h` which defines the max number of pending
    connections for a socket
* blocking:
  * if the client calls `connect()` first, the function blocks until the server calls `accept()`
  * if the server calls `accept()` first, the function blocks until the client calls `connect()`

other utilities
* `socketpair()`: a shorthand to intialize a pair of socket file descriptors -- similar to `pipe()`
  * the sockets are not bound to any address, and is thus safer
* `getsockname()`
* `getpeername()`

## Domains

### UNIX domain

* the socket must be bind to a non-existing pathname
* besides pathname, abstract socket name is also possible on Linux: the first byte being '\0' and
  all following bytes are treated as its unique name in entirety (not NULL-terminated)
  * no need to access file system
  * no need to clean up the binding file (the name is automatically recycled on closing)
  * some buggy code might inadvertently use abstract socket name (e.g. the address is set to 0 but
    never assigned a pathname)
* when the socket is no longer required, user should remove it with `unlink()` or `remove()`
* `bind()` creates socket file, whose permission can be adjusted by `umask()`
* socket file permissions: stream socket connect and datagram write require "write" permission

### Internet domain

* `INADDR_LOOPBACK`: one of the IPv4 loopback address `127.0.0.1`
* `INADDR_ANY`: IPv4 wildcard address (implementation-defined `0.0.0.0`)
  * used in filters to specify that packets sent to all IPs of this hosts are accepted

ports
* port taxonomy by Internet Assigned Numbers Authority (IANA)
  * well known ports: strictly managed by IANA, 0 to 1023
  * registered ports: managed loosenly by IANA, a subset of 1024 to 41951
  * well known ports are treated as privileged ports in TCP/IP implementation (
    `CAP_NET_BIND_SERVICE` required)
  * dynamic / private ports: always managed by the local OS, 49152 to 65535
* the TCP and UDP ports with the same port number are distinct entities, but in practice they are
  almost always assigned to one service to avoid confusions
* when a process bind itself to port 0, a ephemeral port will be assigned
  * `/proc/sys/net/ipv4/ip_local_port_range`

## Protocols

UDP
* features
  * connectionless & unreliable: same as IP
  * port number support
  * data checksum: 16 bits add-up checksum (weak)
* UDP has no IP fragmentation support, so the application must make sure the packet needs not to be
  fragmentated
  * IPv4 minimum reassembly buffer size: 576 bytes
  * UDP header: 8 bytes
  * IPv4 header: 20 bytes
  * the payload should be no more than 548 bytes if it has no information about minimum path MTU

TCP
* TCP endpoint: TCP information maintained in the kernel
  * state info
  * send buffer
  * receive buffer
* featuress
  * connection-oriented: connection establishment and parameter negotiation required
  * reliable: sequencing, acknowgement, retransmission and timeouts supported
  * fragmentation support: data is broken into segments and each of which contains a checksum
  * congestion control

## Utilities

### Byte Order

most network-specific functions accept and return values in network byte order

network byte order - host byte order conversion
it is a good practice to use such conversion no matter what endian your host machine has
```c
/* <arpa/inet.h> */
uint16_t htons(uint16_t host_uint16);
uint16_t ntohs(uint16_t host_uint16);
/* NOTE the l (long) suffix refers to 32-bit for historical reasons */
uint32_t htonl(uint32_t host_uint32);
uint32_t ntohl(uint32_t host_uint32);
```

marshalling (data serialization)
* examples: XDR, XML or ASCII text (telnet)

### Representations

* binary values
  * IPv4 address: 32-bit value
  * IPv6 address: 128-bit value
* symbolic names
  * hostname: machine identifier, which possibly has multiple IP addresses
  * service name: a symbolic representation of a port number (of a certain host?)
* presentation format
  * IPv4: dotted decimals like 127.0.0.1
  * IPv6: colon-separated hex strings

conversions
* binary representation & presentation format
  * a stands for ascii string; n stands for network, p stands fro presentation format
  * IPv4: `inet_aton` and `inet_ntoa` (deprecated)
  * IPv4 & IPv6: `inet_pton` and `inet_ntop`
* binary representation & symbolic names
  * IP: `gethostbyname` (deprecated), `gethostbyaddr` (deprecated)
  * port: `getservbyname` (deprecated), `getservbyport` (deprecated)
  * IP & port: `getaddrinfo()`, `getnameinfo()`

```c
/* ascii / characters (dotted-decimals) --> network (IPv4) address; (ntoa deprecated) */
int inet_aton(const char *cp, struct in_addr *inp);
/* ascii / characters (dotted-decimals) <--> presentation fromat (IPv4 or IPv6) */
int inet_pton(int af, const char *src, void *dst);
int inet_ntop(int af, const char *src, char *dst, socklen_t size);
```

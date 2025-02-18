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

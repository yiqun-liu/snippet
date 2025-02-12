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
  * use UDP in `INET` & `INET6` domain

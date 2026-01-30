# memdev - Memory Device with Configurable Cache Attributes

A Linux kernel module that creates a character device allowing user space to
map memory with different cache attributes.

NOTE: AArch64 does not allow mapping a memory location with different memory attributes. The memory of memdev comes from Linux Kernel's buddy allocator, which is normally mapped as normal cacheable memory in kernel's direct mapping. Mapping it with noncacheable attributes is against the ISA requirement and should NOT be used in production.

## Features

- Page allocations from kernel buddy allocator (order-0 pages)
- NUMA-aware allocation respecting process mempolicy
  - Supports: MPOL_DEFAULT, MPOL_LOCAL, MPOL_PREFERRED
  - Rejects: MPOL_INTERLEAVE, MPOL_BIND with error
  - Falls back gracefully on non-NUMA systems
- Three memory attributes (set at module load):
  - `normal_cacheable`: Standard cached memory
  - `normal_noncacheable`: Write-combining (weakly ordered)
  - `device_noncacheable`: Strongly ordered (device-like)
- Module refcount prevents unload while mapped
- Multiple mappings per file handle
- Linear search tracking (simple, O(n) lookup)

## Building

```bash
make          # Build kernel module
make test     # Build test program
```

## Usage

### Loading the Module

```bash
# Default (normal_cacheable)
sudo insmod memdev.ko

# With specific attribute
sudo insmod memdev.ko attr=normal_noncacheable
sudo insmod memdev.ko attr=device_noncacheable

# Check current attribute
cat /sys/module/memdev/parameters/attr
```

### Using the Device

```c
int fd = open("/dev/memdev", O_RDWR);
void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

// Use memory...
munmap(addr, size);
close(fd);
```

## Test Program

```bash
# Run test (default 256MB)
sudo ./test/test_memdev

# Custom size
sudo ./test/test_memdev -s 1G

# Help
./test/test_memdev -h
```

Output:
```
memdev test: attr=normal_noncacheable, size=256 MB

Bandwidth test:
  write: 3.12 GB/s (81.9 us for 256 MB)
  read:  1.34 GB/s (191.0 us for 256 MB)

Latency test:
  random access: 890 ns/access (890.5 ms total)
```

## Unloading

```bash
sudo rmmod memdev
```

Refuses if mappings still active.

## Implementation

### Memory Allocation

- Uses `alloc_pages_bulk_array_node()` for NUMA-aware page allocation from buddy allocator
- `vm_insert_pages()` for user-space mapping with configurable cache attributes
- Falls back to single-node allocation on non-NUMA kernels

### Tracking

- Single global `file_list` with `dev_lock` mutex
- VMA close callback linear search for cleanup
- Simple and sufficient for demo purposes

## Kernel Version

Tested on Linux 6.8.x.

## License

GPL

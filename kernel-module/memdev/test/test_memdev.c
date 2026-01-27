#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <errno.h>

#define DEVICE_PATH     "/dev/memdev"
#define PARAM_PATH      "/sys/module/memdev/parameters/attr"
#define PAGE_SIZE       4096

static void read_attr(char *buf, size_t len)
{
    int fd = open(PARAM_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open param");
        exit(1);
    }
    ssize_t n = read(fd, buf, len - 1);
    if (n > 0) {
        buf[n] = '\0';
        if (buf[n - 1] == '\n')
            buf[n - 1] = '\0';
    }
    close(fd);
}

static void test_bandwidth(void *addr, size_t size)
{
    struct timeval start, end;
    double time_us;
    size_t i, npages = size / PAGE_SIZE;
    volatile uint8_t *p = addr;

    memset((void *)addr, 0xAA, size);

    gettimeofday(&start, NULL);
    for (i = 0; i < npages; i++)
        p[i * PAGE_SIZE] = (uint8_t)(i & 0xFF);
    gettimeofday(&end, NULL);

    time_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
    double bw = size / time_us / 1e6;
    printf("  write: %.2f MB/s (%.1f us for %zu MB)\n", bw, time_us, size / (1024 * 1024));

    volatile uint8_t sum = 0;
    gettimeofday(&start, NULL);
    for (i = 0; i < npages; i++)
        sum += p[i * PAGE_SIZE];
    gettimeofday(&end, NULL);

    time_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
    bw = size / time_us / 1e6;
    printf("  read:  %.2f MB/s (%.1f us for %zu MB)\n", bw, time_us, size / (1024 * 1024));
    (void)sum;
}

static void test_latency(void *addr, size_t size)
{
    struct timeval start, end;
    double time_us;
    size_t i, n = 1000000;
    uint64_t *offsets = malloc(n * sizeof(uint64_t));
    volatile uint8_t sum = 0;
    uint8_t *p = addr;

    srand(42);
    for (i = 0; i < n; i++)
        offsets[i] = ((size_t)rand() % (size / PAGE_SIZE)) * PAGE_SIZE;

    gettimeofday(&start, NULL);
    for (i = 0; i < n; i++)
        sum += p[offsets[i]];
    gettimeofday(&end, NULL);

    time_us = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
    printf("  random access: %.0f ns/access (%.1f ms total)\n", time_us * 1000 / n, time_us / 1000);
    (void)sum;
    free(offsets);
}

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("  -s <size>   Size (default: 256M)\n");
    printf("  -h          Help\n");
}

int main(int argc, char *argv[])
{
    int opt, fd;
    size_t size = 256 * 1024 * 1024;
    char attr[64];
    void *addr;

    while ((opt = getopt(argc, argv, "s:h")) != -1) {
        switch (opt) {
        case 's':
            if (strchr(optarg, 'M')) {
                size = strtoul(optarg, NULL, 10) * 1024 * 1024;
            } else if (strchr(optarg, 'G')) {
                size = strtoul(optarg, NULL, 10) * 1024 * 1024 * 1024;
            } else {
                size = strtoul(optarg, NULL, 10);
            }
            break;
        case 'h':
        default:
            usage(argv[0]);
            return opt == 'h' ? 0 : 1;
        }
    }

    if (access(DEVICE_PATH, F_OK) != 0) {
        fprintf(stderr, "Error: %s not found. Is memdev module loaded?\n", DEVICE_PATH);
        return 1;
    }

    read_attr(attr, sizeof(attr));

    printf("memdev test: attr=%s, size=%zu MB\n\n", attr, size / (1024 * 1024));

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    printf("Bandwidth test:\n");
    test_bandwidth(addr, size);

    printf("\nLatency test:\n");
    test_latency(addr, size);

    munmap(addr, size);
    close(fd);
    return 0;
}

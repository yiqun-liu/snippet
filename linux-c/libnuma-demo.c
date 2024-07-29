/*
 * A demo of libnuma.
 *
 * References
 *   - kernel doc: Documentaiton/vm/numa_memory_policy.txt
 *   - man numa
 *
 * Notes:
 *   - set_mempolicy & get_mempolicy: task level memory policy
 *   - mbind: VMA level memory policy
 *   - numctl: shell / CLI tools
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <sys/mman.h>

#define PAGE_COUNT	2

int verify_page_numa(void *buf, int num_pages, int nodeid)
{
	int ret, page_size = getpagesize();

	void **pages = (void *)malloc(sizeof(void*) * num_pages);
	if (pages == NULL) {
		fprintf(stderr, "%s: failed to malloc.\n", __func__);
		return ENOMEM;
	}

	int *status = (int *)malloc(sizeof(int) * num_pages);
	if (status == NULL) {
		fprintf(stderr, "%s: failed to malloc.\n", __func__);
		return ENOMEM;
	}

	for (int i = 0; i < num_pages; i++)
		pages[i] = (void*)((unsigned long)buf + page_size);

	ret = move_pages(0, num_pages, pages, NULL, status, 0);
	if (ret == -1) {
		ret = errno;
		fprintf(stderr, "%s: failed to query via move_pages, errmsg=%s.\n", __func__,
			strerror(errno));
		goto out_free;
	}
	for (int i = 0; i < num_pages; i++) {
		ret = -1;
		if (status[i] != nodeid) {
			fprintf(stderr, "%s: unexpected page NUMA. id %d, expect %d, actual %d.\n",
				__func__, i, nodeid, status[i]);
			goto out_free;
		}
	}
	ret = 0;

out_free:
	free(status);
	return ret;
}

int mbind_demo(int nodeid)
{
	int ret, page_size, num_nodes;
	void *buf;
	struct bitmask *node_mask;

	page_size = getpagesize();
	num_nodes = numa_max_node() + 1;
	if (num_nodes == 1) {
		/* This is a workaround. MPOL_BIND mode does not work well with num_nodes=1.
		 * See mm/mempolicy.c: get_nodes() and mpol_new(). */
		num_nodes++;
	}

	buf = mmap(NULL, page_size * PAGE_COUNT, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buf == MAP_FAILED) {
		ret = errno;
		fprintf(stderr, "%s: failed to mmap page, errmsg=%s.\n", __func__, strerror(errno));
		return ret;
	}

	node_mask = numa_bitmask_alloc(num_nodes);
	if (node_mask == NULL) {
		ret = ENOMEM;
		fprintf(stderr, "%s: failed to alloc bitmask.\n", __func__);
		goto out_unmap;
	}
	numa_bitmask_setbit(node_mask, nodeid);

	/*
	 * @mode: MPOL_DEFAULT, MPOL_BIND, MPOL_INTERLEAVE, MPOL_WEIGHTED_INTERLEAVE,
	 *   MPOL_PREFERRED, or MPOL_LOCAL
	 * @size: if you uses the libnuma bitmask, remember to plus its size by 1
	 * @maxnode: this means the maximum number of nodes, rather than the maximum node id. Kernel
	 *   sanitize bitmask using this value.
	 * @flags: by default mbind only affects new allocations. MPOL_MF_MOVE overrides such config
	 */
	ret = mbind(buf, page_size * PAGE_COUNT, MPOL_BIND, node_mask->maskp, num_nodes,
		MPOL_MF_MOVE | MPOL_MF_STRICT);
	if (ret == -1) {
		ret = errno;
		fprintf(stderr, "%s: failed to mbind, errmsg=%s.\n", __func__, strerror(errno));
		goto out_free_bitmask;
	}

	/* access */
	memset(buf, 0, page_size * PAGE_COUNT);

	/* verify */
	ret = verify_page_numa(buf, PAGE_COUNT, nodeid);

out_free_bitmask:
	numa_bitmask_free(node_mask);
out_unmap:
	munmap(buf, page_size);

	printf("%s done. ret=%d.\n", __func__, ret);
	return ret;
}

int numa_alloc_onnode_demo(int nodeid)
{
	int page_size, ret;
	void *buf;

	page_size = getpagesize();
	buf = numa_alloc_onnode(page_size * PAGE_COUNT, nodeid);

	/* access */
	memset(buf, 0, page_size * PAGE_COUNT);

	/* verify */
	ret = verify_page_numa(buf, PAGE_COUNT, nodeid);

	numa_free(buf, page_size * PAGE_COUNT);

	printf("%s done. ret=%d.\n", __func__, ret);
	return ret;
}

int main()
{
	int err_count = 0;
	int nodeid, max_nodeid;

	if (numa_available() == -1) {
		fprintf(stderr, "libnuma not supported on this system.\n");
		return -1;
	}
	max_nodeid = numa_max_node();
	nodeid = max_nodeid;

	err_count += !!mbind_demo(nodeid);
	if (--nodeid < 0)
		nodeid = max_nodeid;

	err_count += !!numa_alloc_onnode_demo(nodeid);
	if (--nodeid < 0)
		nodeid = max_nodeid;

	printf("All tests done.\n");
	return err_count;
}

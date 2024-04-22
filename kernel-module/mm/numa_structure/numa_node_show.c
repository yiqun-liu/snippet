/*
 * A demo of NUMA-related data structure accessor.
 * The locking rules have not been examined in details so there may be some concurrency risks
 */

#include <linux/init.h>
#include <linux/module.h>

#include <linux/mmzone.h>

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* backward-compatibility */
#ifndef MAX_PAGE_ORDER
#define MAX_PAGE_ORDER MAX_ORDER
#endif

static const char *zone_type_strs[] = {
#ifdef CONFIG_ZONE_DMA
	[ZONE_DMA] = "ZONE_DMA",
#endif
#ifdef CONFIG_ZONE_DMA32
	[ZONE_DMA32] = "ZONE_DMA32",
#endif

	[ZONE_NORMAL] = "ZONE_NORMAL",

#ifdef CONFIG_HIGHMEM
	[ZONE_HIGHMEM] = "ZONE_HIGHMEM",
#endif

	[ZONE_MOVABLE] = "ZONE_MOVABLE",

#ifdef CONFIG_ZONE_DEVICE
	[ZONE_DEVICE] = "ZONE_DEVICE",
#endif
};

static void show_numa_instances(void)
{
	/*
	 * the stable API to access struct pglist_data is through the nid, rather than through the
	 * configuration-dependent data structures like contig_page_data or node_data[].
	 * <linux/mmzone.h> defines interfaces like NODE_DATA(nid), node_present_pages(nid),
	 * node_end_pfn(nid).
	 */
	unsigned int nid;
	_Bool direct_access = false;

	/* show spec info */
	unsigned long node_size = sizeof(struct pglist_data);
	unsigned long zone_size = sizeof(struct zone);
	unsigned long zonelist_size = sizeof(struct zonelist);
	unsigned long free_area_size = sizeof(struct free_area);
	pr_info("sizeof(struct pglist_data)=0x%lx=%lu, MAX_NUMNODES=%d, prod=0x%lx",
		node_size, node_size, MAX_NUMNODES, node_size * MAX_NUMNODES);
	pr_info("sizeof(struct zone)=0x%lx=%lu, MAX_NR_ZONES=%d, prod=0x%lx",
		zone_size, zone_size, MAX_NR_ZONES, zone_size * MAX_NR_ZONES);
	pr_info("sizeof(struct zonelist)=0x%lx=%lu, MAX_ZONELISTS=%d, prod=0x%lx",
		zonelist_size, zonelist_size, MAX_ZONELISTS, zonelist_size * MAX_ZONELISTS);
	pr_info("sizeof(struct free_area)=0x%lx=%lu, MAX_PAGE_ORDER=%d, adjusted_prod=x0%lx",
		free_area_size, free_area_size, MAX_PAGE_ORDER,
		free_area_size * (MAX_PAGE_ORDER + 1));

	/* show each node */
	for_each_online_node(nid) {
		long i;

		/* show general node info */
		if (direct_access) {
			struct pglist_data *node = NODE_DATA(nid);
			pr_info("[node] ptr=0x%px node_id=%d, nr_zones=%d, pfn={start=0x%lx, "
				"end=0x%lx, #present=0x%lx, #spanned=0x%lx}", node, node->node_id,
				node->nr_zones, node->node_start_pfn,
				node->node_start_pfn + node->node_spanned_pages,
				node->node_present_pages, node->node_spanned_pages);
		} else {
			pr_info("[node] ptr=0x%px node_id=%d, nr_zones=%d, pfn={start=0x%lx, "
				"end=0x%lx, #present=0x%lx, #spanned=0x%lx}", NODE_DATA(nid), nid,
				NODE_DATA(nid)->nr_zones, node_start_pfn(nid), node_end_pfn(nid),
				node_present_pages(nid), node_spanned_pages(nid));
		}

		/* show zone info */
		for (i = 0; i < MAX_NR_ZONES; i++) {
			struct zone *zone = NODE_DATA(nid)->node_zones + i;
			if (i != zone_idx(zone)) {
				pr_err("Unexpected zone index... Abort.");
				return;
			}
			pr_info("\tzone[%ld]: ptr=0x%px type=%s initialized=%d populated=%d",
				i, zone, zone_type_strs[zone_idx(zone)], zone_is_initialized(zone),
				populated_zone(zone));
			if (!populated_zone(zone))
				continue;
			pr_cont("\t | pfn=[0x%lx, 0x%lx), pages={managed=0x%lx, cma=0x%lx, "
				"present=0x%lx, spanned=0x%lx} name=%s, flags=0x%lx, contiguous=%d",
				zone->zone_start_pfn, zone_end_pfn(zone), zone_managed_pages(zone),
				zone_cma_pages(zone), zone->present_pages, zone->spanned_pages,
				zone->name, zone->flags, zone->contiguous);
		}

		direct_access = !direct_access;
	}
}
static void pr_node_state_mask(nodemask_t *nodemask)
{
	int i;
	pr_cont(" node_mask =");
	for (i = 0; i < BITS_TO_LONGS(MAX_NUMNODES); i++)
		pr_cont(" 0x%lx", nodemask->bits[i]);
}
static void show_node_states(void)
{
	pr_info("[NODE_STATES]: start");
	pr_info("NR_NODE_STATES = %d, output some of the states I care about...", NR_NODE_STATES);
	pr_info("\tN_POSSIBLE\tcount=%d", num_possible_nodes());
	pr_node_state_mask(&node_possible_map);
	pr_info("\tN_ONLINE\tcount=%d", num_online_nodes());
	pr_node_state_mask(&node_online_map);
	pr_info("\tN_NORMAL_MEMORY\tcount=%d", num_node_state(N_NORMAL_MEMORY));
	pr_node_state_mask(&node_states[N_NORMAL_MEMORY]);
	pr_info("\tN_CPU\tcount=%d", num_node_state(N_CPU));
	pr_node_state_mask(&node_states[N_CPU]);
	pr_info("\tN_MEMORY\tcount=%d", num_node_state(N_MEMORY));
	pr_node_state_mask(&node_states[N_MEMORY]);
	pr_info("[NODE_STATES]: end");
}

#ifndef CONFIG_NUMA
extern struct pglist_data contig_page_data;
static void show_all(void)
{
	pr_info("[UMA-SHOW]: start");
	show_node_states();
	pr_info("contig_page_data=0x%px, NODE_DATA(0)=0x%px, match=%d", &contig_page_data,
		NODE_DATA(0),
	&contig_page_data == NODE_DATA(0));
	show_numa_instances();
	pr_info("[UMA-SHOW]: end");
}
#else
extern struct pglist_data *node_data[];
static void show_all(void)
{
	pr_info("[NUMA-SHOW]: start");
	show_node_states();
	pr_info("node_data=0x%px, node_data[0]=0x%px NODE_DATA(0)=0x%px, match=%d", node_data,
		node_data[0], NODE_DATA(0), node_data[0] == NODE_DATA(0));
	show_numa_instances();
	pr_info("[NUMA-SHOW]: end\n");
}
#endif

static int __init numa_show_demo_init(void)
{
	show_all();
	/* we do not really need the module to be plugged in */
	return -EPERM;
}

static void __exit numa_show_demo_exit(void)
{
	return;
}

module_init(numa_show_demo_init);
module_exit(numa_show_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A demo of NUMA-related data structure accessor");

/*
 * A demo of NETLINK GENERIC interface
 *
 * References:
 *   - kernel's NETLINK documents
 *   - Linux Foundation Wiki: generic_netlink_howto
 *   - patches from ethtool
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/genetlink.h>

#include "genl_demo_uapi.h"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define DEMO_GENL_VERSION	(1)

/* generic netlink family */
static struct genl_family demo_genl_family;

/* event / notifications ID */
static atomic_t g_event_id;

/* kernel multicast notifications */
enum demo_multicast_group {
	DEMO_MCGRP_MONITOR,
};
static const struct genl_multicast_group demo_genl_mcgrps[] = {
	[DEMO_MCGRP_MONITOR] = { .name = DEMO_MCGRP_MONITOR_NAME },
};
static int demo_nla_put_str(struct sk_buff *skb, u16 attrtype, const char *str, unsigned int len)
{
	struct nlattr *attr;
	char *terminator;

	attr = nla_reserve(skb, attrtype, len + 1);
	if (!attr)
		return -EMSGSIZE;

	memcpy(nla_data(attr), str, len);
	terminator = (char*)nla_data(attr) + len;
	*terminator = '\0';

	return 0;
}
static void demo_event_notify(u32 event_id, u32 event_type, u64 timestamp, const char *component)
{
	struct sk_buff *skb;
	void *payload;
	unsigned int header_size, attrs_size, component_str_len;

	/* evaluate the packet size before memory allocation */
	header_size = 0;
	component_str_len = strnlen(component, DEMO_MAX_STR_LEN);
	attrs_size = nla_total_size(sizeof(s32)) + nla_total_size(sizeof(u64)) +
		nla_total_size(component_str_len + 1);

	/* allocate the socket buffer */
	skb = genlmsg_new(header_size + attrs_size, GFP_KERNEL);
	if (skb == NULL) {
		pr_err("genlmsg_new(%d) failed.\n", header_size + attrs_size);
		return;
	}

	/* write the netlink header and generic netlink header */
	/* kernel doc recommends that asynchronous notifications use seq = 0. */
	/* skb, portid, seq, family, flags, cmd */
	payload = genlmsg_put(skb, 0, 0, &demo_genl_family, 0, DEMO_K_CMD_EVENT_NOTIFY);
	if (payload == NULL) {
		pr_err("genlmsg_put() failed. Notification packet filling failed.\n");
		return;
	}

	/* write the custom header: not included in this demo */

	/* write the attributes: event_type, timestamp and component */
	if (nla_put_u32(skb, DEMO_K_ATTR_EVENT_ID, event_id) ||
		nla_put_u32(skb, DEMO_K_ATTR_EVENT_TYPE, event_type) ||
		nla_put_u64_64bit(skb, DEMO_K_ATTR_TIMESTAMP, timestamp, DEMO_K_ATTR_PAD) ||
		demo_nla_put_str(skb, DEMO_K_ATTR_COMPONENT, component, component_str_len)) {

		pr_err("failed to run nla_put().\n");
		/* if there were a return value, the value would be -EMSGSIZE by convention*/
		return;
	}

	/* update the length field in the header */
	genlmsg_end(skb, payload);

	/* family, sk_buff, port_id, group, GFP flags */
	genlmsg_multicast(&demo_genl_family, skb, 0, DEMO_MCGRP_MONITOR, GFP_KERNEL);
}

/* user request handlers */
static struct nla_policy demo_report_attr_policy[] = {
	[DEMO_U_ATTR_EVENT_TYPE] = { .type = NLA_U32 },
	[DEMO_U_ATTR_TIMESTAMP] = { .type = NLA_U64 },
};
static int demo_event_report(struct sk_buff *skb, struct genl_info *info)
{
	char component[DEMO_MAX_STR_LEN + 1];
	u32 event_type;
	u64 timestamp;

	if (!info->attrs[DEMO_U_ATTR_EVENT_TYPE]) {
		pr_err("mandatory attribute event_type not found.\n");
		return -EINVAL;
	}
	if (!info->attrs[DEMO_U_ATTR_TIMESTAMP]) {
		pr_err("mandatory attribute timestamp not found.\n");
		return -EINVAL;
	}

	event_type = nla_get_u32(info->attrs[DEMO_U_ATTR_EVENT_TYPE]);
	timestamp = nla_get_u64(info->attrs[DEMO_U_ATTR_TIMESTAMP]);

	snprintf(component, sizeof(component), "port-%u", info->snd_portid);
	demo_event_notify((u32)atomic_fetch_inc(&g_event_id), event_type, timestamp, component);

	return 0;
}
static const struct genl_ops demo_genl_ops[] = {
	{
		.cmd = DEMO_U_CMD_EVENT_REPORT,
		.doit = demo_event_report,
		.policy = demo_report_attr_policy,
		.maxattr = ARRAY_SIZE(demo_report_attr_policy)
	},
};

static struct genl_family demo_genl_family __ro_after_init = {
	.hdrsize = sizeof(struct demo_genl_header),
	.name = DEMO_GENL_NAME,
	.version = DEMO_GENL_VERSION,
	.parallel_ops = true,
	.ops = demo_genl_ops,
	.n_ops = ARRAY_SIZE(demo_genl_ops),
	.mcgrps = demo_genl_mcgrps,
	.n_mcgrps = ARRAY_SIZE(demo_genl_mcgrps),
	.module = THIS_MODULE,
};

static int __init demo_genl_init(void)
{
	int ret;

	ret = genl_register_family(&demo_genl_family);
	if (ret < 0) {
		pr_err("failed to register demo genl family, ret=%d.\n", ret);
		return ret;
	}

	atomic_set(&g_event_id, 1);

	/* print family information */
	pr_info("genl_family's key static parameters: n_ops=%u, n_mcgrps=%u, ops.max_attr=[%u].\n",
		demo_genl_family.n_ops, demo_genl_family.n_mcgrps, demo_genl_family.ops[0].maxattr);
	pr_info("genl_family's key dynamic properties: id=%d, mcgrp_offset=%u.\n",
		demo_genl_family.id, demo_genl_family.mcgrp_offset);

	pr_info("Generic netlink family %s v%d registered.\n", DEMO_GENL_NAME, DEMO_GENL_VERSION);

	return 0;
}

static void __exit demo_genl_exit(void)
{
	int ret;
	ret = genl_unregister_family(&demo_genl_family);
	if (ret)
		pr_err("failed to unregister demo genl family.\n");

	pr_info("Generic netlink family %s v%d deregistered.\n", DEMO_GENL_NAME, DEMO_GENL_VERSION);
}

module_init(demo_genl_init);
module_exit(demo_genl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yiqun Liu <yiqun.liu.dev@outlook.com>");
MODULE_DESCRIPTION("A demo of generic netlink family");

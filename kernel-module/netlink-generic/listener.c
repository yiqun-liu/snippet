/*
 * A demo of libnl user library: message receiver
 *
 * References:
 *   - libnl document by Thomas Graf (https://infradead.org/~tgr/libnl/doc/core.html)
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "genl_demo_uapi.h"

#define MOD_NAME		"listener"
#define pr_info(fmt, ...)	fprintf(stdout, "[INFO]  " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)
#define pr_err(fmt, ...)	fprintf(stderr, "[ERROR] " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)

struct nl_sock *g_nl_sock;

static unsigned long get_time_us(void)
{
	struct timespec spec;
	int ret = clock_gettime(CLOCK_REALTIME, &spec);
	if (ret == -1) {
		pr_err("failed to get time, errmsg=%s.", strerror(errno));
		exit(ret);
	}
	return spec.tv_sec * 1000000UL + spec.tv_nsec / 1000;
}

static struct nl_sock* get_genl_multicast_socket(const char *family_name, const char *mcgrp_name)
{
	int ret, mcgrp_id;
	struct nl_sock *nl_sock;

	nl_sock = nl_socket_alloc();
	if (nl_sock == NULL) {
		pr_err("nl_socket_alloc() failed.");
		exit(-1);
	}

	ret = nl_connect(nl_sock, NETLINK_GENERIC);
	if (ret < 0) {
		pr_err("failed to connect with NETLINK_GENERIC, errmsg=%s.", nl_geterror(ret));
		exit(ret);
	}

	ret = genl_ctrl_resolve_grp(nl_sock, family_name, mcgrp_name);
	if (ret < 0) {
		pr_err("failed to resolve multicast group id. family=%s, group=%s, errmsg=%s.",
			family_name, mcgrp_name, nl_geterror(ret));
		exit(ret);
	}
	mcgrp_id = ret;

	ret = nl_socket_add_membership(nl_sock, mcgrp_id);
	if (ret < 0) {
		pr_err("nl_socket_add_membership() failed. group-id=%d, errmsg=%s.", mcgrp_id,
			nl_geterror(ret));
		exit(ret);
	}

	pr_info("%s() called. retval=%p", __func__, nl_sock);

	/* notification-specific settings */

	/* Sequence number has no meaning in the one-sided kernel notifications. libnl performs
	 * sequence number check as in request-reply sessions -- seq_check should be disabled.
	 * In libnl, seq-check is done through callbacks. Therefore, if you are going to replace the
	 * the callback set as a whole, the order matters. Replacing the callback set could undo
	 * the effects of the following statements. The current implementation chose the callback
	 * set method, we we leave the disable statement here just as a memo. */
	/* nl_socket_disable_seq_check(nl_sock); */

	/* Auto-ACK also does not work with notifications. */
	nl_socket_disable_auto_ack(nl_sock);

	return nl_sock;
}

/* NOTE: This triggers direct deallocation. It is not refcount-based. */
static void put_genl_multicast_socket(struct nl_sock *nl_sock)
{
	if (nl_sock == NULL) {
		pr_err("%s() get NULL arg. Not fully initialized.", __func__);
		return;
	}
	nl_close(nl_sock);
	nl_socket_free(nl_sock);

	pr_info("%s() called.", __func__);
}

static struct nla_policy notify_attr_policy[] = {
	[DEMO_K_ATTR_PAD] = { .type = NLA_BINARY },
	[DEMO_K_ATTR_EVENT_TYPE] = { .type = NLA_U32 },
	[DEMO_K_ATTR_TIMESTAMP] = { .type = NLA_U64 },
	[DEMO_K_ATTR_COMPONENT] = { .type = NLA_NUL_STRING },
};

static struct {
	unsigned int event_id;
	unsigned int event_type;
	unsigned long event_time;
	unsigned long receive_time;
	const char *component;
} g_event_info;

static void clear_event_info(void)
{
	if (g_event_info.component)
		free((void*)g_event_info.component);
	memset(&g_event_info, 0, sizeof(g_event_info));
}

/* The canonical way to deal with received messages is through call back. Manually call nl_recv()
 * and parsing function is also feasible. */
static int event_callback(struct nl_msg *nl_msg, void *arg)
{
	struct nlmsghdr *nl_hdr = nlmsg_hdr(nl_msg);
	struct nlattr *attrs[DEMO_K_ATTR_CNT];

	/* validate argument passing (for demo purpose), see monitor_events() */
	if (arg != NULL) {
		pr_err("Wrong argument passed to callback function. Check configuration.");
		exit(-1);
	}

	pr_info("nl_msg received. Dump message ...");
	nl_msg_dump(nl_msg, stdout);
	pr_info("nl_msg dumped (initial state). Start packet parsing ...");

	int ret = genlmsg_parse(nl_hdr, sizeof(struct demo_genl_header), attrs, DEMO_K_ATTR_MAX,
		notify_attr_policy);
	if (ret < 0) {
		pr_err("genlmsg_parse() failed, errmsg=%s.", nl_geterror(ret));
		return ret;
	}

	/* get fields */
	g_event_info.event_id = nla_get_u32(attrs[DEMO_K_ATTR_EVENT_ID]);
	g_event_info.event_type = nla_get_u32(attrs[DEMO_K_ATTR_EVENT_TYPE]);
	g_event_info.event_time = nla_get_u64(attrs[DEMO_K_ATTR_TIMESTAMP]);
	/* nla_get_string() is also an alternative. It returns a pointer into the nl_msg buffer,
	 * which is only accessible in this call back */
	g_event_info.component = nla_strdup(attrs[DEMO_K_ATTR_COMPONENT]);
	if (g_event_info.component == NULL) {
		pr_err("nla_strdup() failed.");
		return -ENOMEM;
	}

	g_event_info.receive_time = get_time_us();

	pr_info("Packet parsed.");

	return 0;
}

/* support two modes: finite repeat mode and infinite repeat mode */
static void monitor_events(int num_events)
{
	/* Here we chose a stateless way to monitor_events. Another way is to do this is modify the
	 * default callback inplace, as done by nl_socket_disable_seq_check(),
	 * nl_socket_modify_cb(), and nl_recvmsgs_default(). */

	/* modify callback */
	struct nl_cb *cb_set = nl_cb_alloc(NL_CB_DEFAULT);
	if (cb_set == NULL) {
		pr_err("nl_cb_alloc(%d) failed.", NL_CB_DEFAULT);
		exit(-1);
	}
	/* replace the callback for valid packets */
	nl_cb_set(cb_set, NL_CB_VALID, NL_CB_CUSTOM, event_callback, NULL);
	nl_cb_set(cb_set, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, NULL, NULL);

	/* start monitor */
	int count = 0;
	pr_info("num_events=%d.", num_events);
	while (++count <= num_events || num_events == -1) {
		/* wait for events */
		int ret = nl_recvmsgs(g_nl_sock, cb_set);
		if (ret < 0) {
			pr_err("nl_recvmsgs() failed, errmsg=%s.", nl_geterror(ret));
			exit(ret);
		}

		/* print results outside the callback -- just for demo purpose. */
		pr_info("event #%d: {type=%d, component=%s, event_time=%lu, "
			"recv_time=%lu, time_diff=%lu us}.", g_event_info.event_id,
			g_event_info.event_type, g_event_info.component, g_event_info.event_time,
			g_event_info.receive_time,
			g_event_info.receive_time - g_event_info.event_time);

		clear_event_info();
	}

	/* release callback set memory */
	nl_cb_put(cb_set);
}

/* quit elegantly when quiting the infinite loop */
static void signal_handler(int signal)
{
	clear_event_info();
	put_genl_multicast_socket(g_nl_sock);
	pr_info("Signaled exit, signal=%d.", signal);
	exit(-1);
}

int main(int argc, char *argv[])
{
	int num_events = -1;
	if (argc > 2) {
		pr_err("arg count unexpected.");
		exit(-EINVAL);
	}
	if (argc == 2)
		num_events = atoi(argv[1]);

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	g_nl_sock = get_genl_multicast_socket(DEMO_GENL_NAME, DEMO_MCGRP_MONITOR_NAME);

	monitor_events(num_events);

	put_genl_multicast_socket(g_nl_sock);

	pr_info("Natural exit.");

	return 0;
}

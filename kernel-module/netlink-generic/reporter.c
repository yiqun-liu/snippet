/*
 * A demo of libnl user library: message sender
 *
 * References:
 *   - libnl document by Thomas Graf (https://infradead.org/~tgr/libnl/doc/core.html)
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "genl_demo_uapi.h"

#define MOD_NAME		"reporter"
#define pr_info(fmt, ...)	fprintf(stdout, "[INFO]  " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)
#define pr_err(fmt, ...)	fprintf(stderr, "[ERROR] " MOD_NAME ": " fmt "\n", ##__VA_ARGS__)

struct {
	/* netlink socket */
	struct nl_sock *nl_sock;

	/* generic netlink family identifier */
	int genl_family_id;
} ctx;

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

static void init_genl_ctx(void)
{
	int ret;

	ctx.nl_sock = nl_socket_alloc();
	if (ctx.nl_sock == NULL) {
		pr_err("nl_socket_alloc() failed.");
		exit(-ENOMEM);
	}

	ret = nl_connect(ctx.nl_sock, NETLINK_GENERIC);
	if (ret < 0) {
		pr_err("failed to connect with NETLINK_GENERIC, errmsg=%s.", nl_geterror(ret));
		exit(ret);
	}

	/* The first thing to do is to communicate with the kernel generic netlink control service
	 * and learn the netlink family information, for example:
	 *   - the dynamically allocated family ID, which we should put in nlmsg_type field in
	 *     NLMSG's header
	 *   - the multicast group ID to receive notifications
	 *   - the family header size expected by the kernel
	 *
	 * In most use cases, We only need one or two pieces of information, especially for those
	 * modules with stable interfaces. A set of simplified interfaces are provided by libnl:
	 *   - genl_ctrl_resolve(genl_socket, family_name)
	 *     returns family ID
	 *   - genl_ctrl_resolve_grp(genl_socket, family_name, group_name)
	 *     returns multicast group ID (NOTE: we don't need a family ID to listen on
	 *     a multicast group)
	 *
	 * Low level manipulation of struct genl_family is possible with libnl, see the
	 * implementation of genl_ctrl_resolve() if it is really necessary. */

	ret = genl_ctrl_resolve(ctx.nl_sock, DEMO_GENL_NAME);
	if (ret < 0) {
		pr_err("failed to resolve family id. name=%s, errmsg=%s.", DEMO_GENL_NAME,
			nl_geterror(ret));
		exit(ret);
	}
	ctx.genl_family_id = ret;

	pr_info("%s() called. Family ID = %d.", __func__, ctx.genl_family_id);
}

void shutdown_genl_ctx(void)
{
	if (ctx.nl_sock == NULL) {
		pr_err("netlink socket not initialized or closed already.");
		return;
	}
	nl_close(ctx.nl_sock);
	nl_socket_free(ctx.nl_sock);
	ctx.nl_sock = 0;
	ctx.genl_family_id = 0;

	pr_info("%s() called.", __func__);
}

void print_genl_msg_layout(struct nl_msg *nl_msg)
{
	struct nlmsghdr *nl_hdr = nlmsg_hdr(nl_msg);
	struct genlmsghdr *genl_hdr = genlmsg_hdr(nl_hdr);
	struct nlattr *genl_attr = genlmsg_data(genl_hdr);
	pr_info("rough layout of: nlmsg=%p", nl_msg);
	pr_info("\tnlmsg_hdr(nl_msg)=%p (+0x%lx)", nl_hdr,
		(unsigned long)nl_hdr - (unsigned long)nl_msg);
	pr_info("\t\tdata=%p, datalen=0x%x", nlmsg_data(nl_hdr), nlmsg_datalen(nl_hdr));
	pr_info("\tgenlmsg_hdr(nlmsg_hdr)=%p (+0x%lx), genlmsg_len(genl_hdr)=0x%x.",
		genl_hdr, (unsigned long)genl_hdr - (unsigned long)nl_hdr, genlmsg_len(genl_hdr));
	pr_info("\tgenlmsg_data(genl_hdr)=%p (+0x%lx).", genl_attr,
		(unsigned long)genl_attr - (unsigned long)genl_hdr);
}

static void report_event(unsigned int event_type)
{
	struct nl_msg *nl_msg;
	struct demo_genl_header *demo_header;

	unsigned long report_time = get_time_us();

	/* use the default message size (PAGE_SIZE), consider using  nlmsg_alloc_size(size) or
	 * nlmsg_set_default_size(size) if this default setting does not work well. */
	nl_msg = nlmsg_alloc();
	if (nl_msg == NULL) {
		pr_err("failed to allocate nelink message buffer.");
		exit(-1);
	}

	pr_info("nl_msg allocated. Dump initial state ...");
	nl_msg_dump(nl_msg, stdout);
	pr_info("nl_msg dumped (initial state). Start packet construction ...");

	/* write netlink generic header */
	demo_header = genlmsg_put(nl_msg, NL_AUTO_PORT, NL_AUTO_SEQ, ctx.genl_family_id,
		sizeof(struct demo_genl_header), 0, DEMO_U_CMD_EVENT_REPORT, DEMO_GENL_API_VERSION);
	pr_info("nlmsg=%p, private_header=%p (offset=0x%lx)", nl_msg, demo_header,
		(unsigned long)demo_header - (unsigned long)nl_msg);

	/* write private header: skipped, in this example our header is of size 0 */

	/* write attribute: use libnl's (not libnl-ge's) helper */
	nla_put_u32(nl_msg, DEMO_U_ATTR_EVENT_TYPE, event_type);
	nla_put_u64(nl_msg, DEMO_U_ATTR_TIMESTAMP, report_time);

	/* write non-attribute payload: not payload included in this demo */

	pr_info("Packet constructed. Dump final state ...");
	nl_msg_dump(nl_msg, stdout);
	print_genl_msg_layout(nl_msg);
	pr_info("Final state dumped.");

	/* send to kernel */
	int ret = nl_send_auto(ctx.nl_sock, nl_msg);
	if (ret < 0) {
		pr_err("failed to send netlink message, errmsg=%s.", nl_geterror(ret));
		exit(ret);
	}
	pr_info("Packet delivered.");

	nlmsg_free(nl_msg);
}

int main(int argc, char *argv[])
{
	int event_type;

	if (argc != 2) {
		pr_err("usage: ./reporter $EVENT_TYPE");
		return -EINVAL;
	}
	event_type = atoi(argv[1]);

	init_genl_ctx();

	report_event(event_type);

	shutdown_genl_ctx();

	return 0;
}

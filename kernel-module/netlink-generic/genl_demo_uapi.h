#ifndef GENETLNK_UAPI_H
#define GENETLNK_UAPI_H

/* identifier used to match generic netlink family with the user space clients */
#define DEMO_GENL_NAME		"demo_genl"
#define DEMO_GENL_API_VERSION	(1)

#define DEMO_MCGRP_MONITOR_NAME	"demo_monitor"

/* string size: NULL terminator excluded */
#define DEMO_MAX_STR_LEN	(63)

struct demo_genl_header {
};

/* message / command types */
/* user to kernel */
enum {
	DEMO_U_CMD_NONE,
	DEMO_U_CMD_EVENT_REPORT,

	/* you can do the range definition the other way, but this the current convention */
	DEMO_U_CMD_CNT,
	DEMO_U_CMD_MAX = DEMO_U_CMD_CNT - 1
};
/* attribute definitions */
enum {
	DEMO_U_ATTR_UNSPEC,
	DEMO_U_ATTR_EVENT_TYPE,		/* u32 */
	DEMO_U_ATTR_TIMESTAMP,		/* u64 */

	DEMO_U_ATTR_CNT,
	DEMO_U_ATTR_MAX = DEMO_U_ATTR_CNT - 1
};

/* kernel to user */
enum {
	DEMO_K_CMD_NONE,
	DEMO_K_CMD_EVENT_NOTIFY,

	DEMO_K_CMD_CNT,
	DEMO_K_CMD_MAX = DEMO_K_CMD_CNT - 1
};
enum {
	DEMO_K_ATTR_UNSPEC,
	DEMO_K_ATTR_PAD,		/* padding attributes are needed for data types like u64 */
	DEMO_K_ATTR_EVENT_ID,		/* u32 */
	DEMO_K_ATTR_EVENT_TYPE,		/* u32 */
	DEMO_K_ATTR_TIMESTAMP,		/* u64 */
	DEMO_K_ATTR_COMPONENT,		/* string */

	DEMO_K_ATTR_CNT,
	DEMO_K_ATTR_MAX = DEMO_K_ATTR_CNT - 1
};

#endif

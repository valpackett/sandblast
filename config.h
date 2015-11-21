#pragma once
#include <unistd.h>
#include <stdbool.h>

#define IPV4_ADDRS_LEN 32
#define IPV6_ADDRS_LEN 32
#define LIMITS_LEN     32
#define MOUNTS_LEN     64

typedef struct {
	char *from;
	char *to;
	bool readonly;
} mount_t;

typedef struct {
	char *jailname;
	char *hostname;
	char *script;
	char *net_iface;
	char *ipv4[IPV4_ADDRS_LEN];
	char *ipv6[IPV6_ADDRS_LEN];
	char *limits[LIMITS_LEN];
	mount_t *mounts[MOUNTS_LEN];
	int8_t securelevel;
} jail_conf_t;

jail_conf_t *load_conf(const char *filename);

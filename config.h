#pragma once

#define MOUNTS_LEN     64
#define LIMITS_LEN     32

typedef struct {
	char *from;
	char *to;
} mount_t;

typedef struct {
	char *jailname;
	char *hostname;
	char *ipv4;
	char *ipv6;
	char *script;
	char *limits[LIMITS_LEN];
	mount_t *mounts[MOUNTS_LEN];
} jail_conf_t;

jail_conf_t *load_conf(char *filename);

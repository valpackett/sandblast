#pragma once

typedef struct {
	char *from;
	char *to;
} mount_t;

typedef struct {
	char *jailname;
	char *hostname;
	char **ipv4;
	char **ipv6;
	char *script;
	uint32_t limit_cpu;
	uint32_t limit_ram;
	uint32_t limit_swap;
	uint32_t limit_openfiles;
	mount_t **mounts;
} jail_conf_t;

jail_conf_t *parse_config(char *filename);

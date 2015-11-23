#pragma once

#define RCTL_ADD "-a"
#define RCTL_REMOVE "-r"

void rctl(const char *operation, const char *rule);
void mount_devfs(const char *to, int16_t devfs_ruleset, void (*on_fail)());
void mount_nullfs(const char *to, const char *from, bool readonly, void (*on_fail)());
void mkdirp(const char *pathname);
void umount(const char *mountpoint);
void ifconfig_alias(const char *interface, const char *net, const char *address);
void ifconfig_unalias(const char *interface, const char *net, const char *address);

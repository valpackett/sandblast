// Sandblast -- the missing simple container tool for FreeBSD
// Copyright (c) 2014-2015 Greg V <greg@unrelenting.technology>
// Available under the ISC license, see the COPYING file

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/jail.h>
#include <sys/procdesc.h>
#include <jail.h>
#include "admin.h"
#include "config.h"
#include "logging.h"
#include "memory.h"

#define TMP_TEMPLATE "/tmp/sandblast.XXXXXXXXXXXXXXXXXXX"

static sem_t *jail_started;
static sem_t *jail_prepared;
static int child_pd;

static char *filename;

// Jail configuration and state -- fully known after jail_started fires
// Accessed from the parent process; jail_id is set from the child
static jail_conf_t *jail_conf;
static bool jail_has_limits = false;
static const char *jail_path;
static int *jail_id;

static char *redir_stdout = "/dev/stdout";
static char *redir_stderr = "/dev/stderr";

static const char *progname;
static bool verbose = false;

////////////////////////////////////////////////////////////////////////

#define sb_jailparam_put(key, val) \
	if (1) { \
		if (jailparam_init(&params[params_cnt], (key)) != 0) \
			die("Could not init jail param %s: %s", (key), jail_errmsg); \
		if (jailparam_import(&params[params_cnt], (val)) != 0) \
			die("Could not import jail param %s: %s", (key), jail_errmsg); \
		params_cnt++; \
	}

void start_jail() {
	freopen(redir_stdout, "w", stdout);
	freopen(redir_stderr, "w", stderr);
	struct jailparam params[7];
	uint32_t params_cnt = 0;

	sb_jailparam_put("path", jail_path);

	char *securelevel_string; asprintf(&securelevel_string, "%d", jail_conf->securelevel);
	sb_jailparam_put("securelevel", securelevel_string);
	free(securelevel_string);

	char *devfs_ruleset_string; asprintf(&devfs_ruleset_string, "%d", jail_conf->devfs_ruleset);
	sb_jailparam_put("devfs_ruleset", devfs_ruleset_string);
	free(devfs_ruleset_string);

	sb_jailparam_put("name", jail_conf->jailname);

	char *ipv4_string = ipaddr_string(jail_conf->ipv4, IPV4_ADDRS_LEN);
	sb_jailparam_put("ip4.addr", ipv4_string);
	free(ipv4_string);

	char *ipv6_string = ipaddr_string(jail_conf->ipv6, IPV6_ADDRS_LEN);
	sb_jailparam_put("ip6.addr", ipv6_string);
	free(ipv6_string);

	sb_jailparam_put("host.hostname", jail_conf->hostname);

	*jail_id = jailparam_set(params, params_cnt, JAIL_CREATE | JAIL_ATTACH);
	printf("%s", jail_errmsg);
	if (*jail_id == -1)
		die_errno("Could not create jail");
	if (chdir("/") != 0)
		die_errno("Could not chdir to jail");
}

void add_limits() {
	for (size_t i = 0; i < LIMITS_LEN; i++) {
		if (jail_conf->limits[i] != NULL) {
			char buf[128];
			if (snprintf(buf, sizeof(buf), "jail:%d:%s", *jail_id, jail_conf->limits[i]) >= 128)
				die("Jail ID + limit too long (when adding limits)");
			rctl(RCTL_ADD, buf);
			jail_has_limits = true;
		}
	}
}

void remove_limits() {
	if (jail_has_limits == false) return;
	char buf[64];
	if (snprintf(buf, sizeof(buf), "jail:%d", *jail_id) >= 64)
		die("Jail ID too long (when removing limits)");
	rctl(RCTL_REMOVE, buf);
}

void alias_ipaddrs() {
	if (jail_conf->net_iface == NULL) return;
	for (size_t i = 0; i < IPV4_ADDRS_LEN; i++)
		if (jail_conf->ipv4[i] != NULL)
			ifconfig_alias(jail_conf->net_iface, "inet", jail_conf->ipv4[i]);
	for (size_t i = 0; i < IPV6_ADDRS_LEN; i++)
		if (jail_conf->ipv6[i] != NULL)
			ifconfig_alias(jail_conf->net_iface, "inet6", jail_conf->ipv6[i]);
}

void unalias_ipaddrs() {
	if (jail_conf->net_iface == NULL) return;
	for (size_t i = 0; i < IPV4_ADDRS_LEN; i++)
		if (jail_conf->ipv4[i] != NULL)
			ifconfig_unalias(jail_conf->net_iface, "inet", jail_conf->ipv4[i]);
	for (size_t i = 0; i < IPV6_ADDRS_LEN; i++)
		if (jail_conf->ipv6[i] != NULL)
			ifconfig_unalias(jail_conf->net_iface, "inet6", jail_conf->ipv6[i]);
}

char* resolve_mountpoint(const char *pathname) {
	char *result; asprintf(&result, "%s/%s", jail_path, pathname);
	return result;
}

void on_failed_mount();

void mount_mounts() {
	for (size_t i = 0; i < MOUNTS_LEN; i++) {
		mount_t *mount = jail_conf->mounts[i];
		if (mount != NULL) {
			char *mountpoint = resolve_mountpoint(mount->to);
			mkdirp(mountpoint);
			mount_nullfs(mountpoint, mount->from, mount->readonly, on_failed_mount);
			free(mountpoint);
		}
	}
	char *mountpoint = resolve_mountpoint("/dev");
	mount_devfs(mountpoint, jail_conf->devfs_ruleset, on_failed_mount);
	free(mountpoint);
}

void unmount_mounts() {
	char *mountpoint = resolve_mountpoint("/dev");
	umount(mountpoint);
	free(mountpoint);
	for (int32_t i = MOUNTS_LEN - 1; i >= 0; i--) {
		mount_t *mount = jail_conf->mounts[i];
		if (mount != NULL) {
			char *mountpoint = resolve_mountpoint(mount->to);
			umount(mountpoint);
			free(mountpoint);
		}
	}
}

void start_process() {
	char *tmpname = copy_string(TMP_TEMPLATE);
	int scriptfd = mkstemp(tmpname); // tmpname IS REPLACED WITH ACTUAL NAME in mkstemp
	if (scriptfd == -1)
		die_errno("Could not create the temp script file");
	if (write(scriptfd, jail_conf->script, strlen(jail_conf->script)) == -1)
		die_errno("Could not write to the temp script file");
	if (fchmod(scriptfd, (uint16_t)strtol("0755", 0, 8)) != 0)
		die_errno("Could not chmod the temp script file");
	if (close(scriptfd) == -1)
		die_errno("Could not close the temp script file");
	if (execve(tmpname, (char *[]){ tmpname, 0 },
				(char *[]){ "PATH=/usr/local/bin:/usr/local/sbin:/usr/games:/usr/bin:/usr/sbin:/bin:/sbin",
				            "LC_ALL=en_US.UTF-8",
				            "LANG=en_US.UTF-8", 0 }) == -1)
		die_errno("Could not spawn the jailed process");
}

// Make sure there are no orphans in the jail
void stop_jail() {
	jail_remove(*jail_id);
}

void handle_sigint() {
	pdkill(child_pd, SIGTERM);
}

void handle_sigterm() {
	pdkill(child_pd, SIGKILL);
}

void start_signal_handlers() {
	signal(SIGQUIT, handle_sigint);
	signal(SIGHUP,  handle_sigint);
	signal(SIGINT,  handle_sigint);
	signal(SIGTERM, handle_sigterm);
}

void wait_for_child() {
	setproctitle("parent of jail `%s` (JID %d)", jail_conf->jailname, *jail_id);
	int child_pid; pdgetpid(child_pd, &child_pid);
	int status; waitpid(child_pid, &status, 0);
	info("Jailed process exited with status %d", status);
}

void start_shared_memory() {
	jail_id = init_shm_int();
	jail_started = init_shm_semaphore();
	jail_prepared = init_shm_semaphore();
}

void free_shared_memory() {
	munmap(jail_id, sizeof(*jail_id));
	munmap(jail_started, sizeof(*jail_started));
	munmap(jail_prepared, sizeof(*jail_prepared));
}

void usage() {
	printf("Usage: %s [-O <stdout>] [-E <stderr>] [-v] config_file\n", progname);
	exit(1);
}

void read_options(int argc, char *argv[]) {
	progname = argv[0];
	int c;
	while ((c = getopt(argc, argv, "O:E:v?h")) != -1) {
		switch (c) {
			case 'O': redir_stdout      = optarg; break;
			case 'E': redir_stderr      = optarg; break;
			case 'v': verbose           = true;   break;
			case '?':
			case 'h':
			default:  usage(); break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();
	filename = argv[0];
	if (strncmp(filename, "-", 1) == 0)
		filename = "/dev/stdin";
}

void start_logging() {
	openlog(progname, LOG_PID | LOG_PERROR | LOG_CONS, LOG_USER);
	setlogmask(LOG_UPTO(verbose ? LOG_INFO : LOG_WARNING));
}

void on_failed_mount() {
	pdkill(child_pd, SIGTERM);
	stop_jail();
	unmount_mounts();
	rmdir(jail_path);
	die("Could not mount");
}

void ensure_root() {
	if (setuid(0) != 0)
		die_errno("Could not get root privileges");
}

int main(int argc, char *argv[]) {
	read_options(argc, argv);
	start_logging();
	start_shared_memory();
	jail_path = mkdtemp(copy_string(TMP_TEMPLATE));
	jail_conf = load_conf(filename);
	ensure_root();
	pid_t child_pid = pdfork(&child_pd, 0);
	if (child_pid == -1)
		die_errno("Could not fork");
	if (child_pid <= 0) { // Child
		start_jail();
		sem_post(jail_started);
		sem_wait(jail_prepared);
		start_process();
	} else { // Parent
		sem_wait(jail_started);
		if (*jail_id != -1) {
			start_signal_handlers();
			mount_mounts();
			alias_ipaddrs();
			add_limits();
			sem_post(jail_prepared);
			wait_for_child();
			remove_limits();
			unalias_ipaddrs();
			unmount_mounts();
			stop_jail();
			rmdir(jail_path);
		}
		free_shared_memory();
	}
	return 0;
}

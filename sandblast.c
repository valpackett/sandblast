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
#include "memory.h"
#include "logging.h"
#include "config.h"

#define TMP_TEMPLATE "/tmp/sandblast.XXXXXXXX"

#define sb_jailparam_start(count) \
	struct jailparam params[count]; \
	unsigned int params_cnt = 0;

#define sb_jailparam_put(key, val) \
	if (1) { \
		if (jailparam_init(&params[params_cnt], (key)) != 0) \
			die("Could not init jail param %s: %s", key, jail_errmsg); \
		if (jailparam_import(&params[params_cnt], (val)) != 0) \
			die("Could not import jail param %s: %s", key, jail_errmsg); \
		params_cnt++; \
	}

#define sb_jailparam_set(flags) \
	jailparam_set(params, params_cnt, (flags))


static sem_t *jail_started;
static int child_pd;

static char *filename;

// Jail configuration and state -- fully known after jail_started fires
// Accessed from the parent process; jail_id is set from the child
static jail_conf_t *jail_conf;
static const char *jail_path;
static int *jail_id;

static char *redir_stdout = "/dev/stdout";
static char *redir_stderr = "/dev/stderr";

static const char *progname;
static bool verbose = false;

////////////////////////////////////////////////////////////////////////

void start_jail() {
	freopen(redir_stdout, "w", stdout);
	freopen(redir_stderr, "w", stderr);
	sb_jailparam_start(4);
	sb_jailparam_put("name", jail_conf->jailname);
	sb_jailparam_put("path", jail_path);
	sb_jailparam_put("ip4.addr", "192.168.122.66");
	sb_jailparam_put("host.hostname", jail_conf->hostname);
	*jail_id = sb_jailparam_set(JAIL_CREATE | JAIL_ATTACH);
	sem_post(jail_started);
	printf("%s", jail_errmsg);
	if (*jail_id == -1)
		die_errno("Could not start jail");
	if (chdir("/") != 0)
		die_errno("Could not chdir to jail");
}

// Make sure there are no orphans in the jail
void stop_jail() {
	jail_remove(*jail_id);
}
 
void start_process() {
	char *tmpname = copy_string(TMP_TEMPLATE);
	int scriptfd = mkstemp(tmpname); // tmpname IS REPLACED WITH ACTUAL NAME in mkstemp
	if (scriptfd == -1)
		die_errno("Could not create the temp script file");
	if (write(scriptfd, jail_conf->script, strlen(jail_conf->script)) == -1)
		die_errno("Could not write to the temp script file");
	if (fchmod(scriptfd, (uint16_t) strtol("0755", 0, 8)) != 0)
		die_errno("Could not chmod the temp script file");
	if (close(scriptfd) == -1)
		die_errno("Could not close the temp script file");
	if (execve(tmpname, (char *[]){ tmpname, 0 },
				(char *[]){ "PATH=/usr/local/bin:/usr/local/sbin:/usr/games:/usr/bin:/usr/sbin:/bin:/sbin",
				            "LC_ALL=en_US.UTF-8",
				            "LANG=en_US.UTF-8", 0 }) == -1)
		die_errno("Could not spawn the jailed process");
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
	setproctitle("[parent of jail %s (JID %d)]", jail_conf->jailname, *jail_id);
	int child_pid; pdgetpid(child_pd, &child_pid);
	int status; waitpid(child_pid, &status, 0);
	info("Jailed process exited with status %d", status);
}

void start_shared_memory() {
	jail_id = init_shm_int();
	jail_started = init_shm_semaphore();
}

void usage() {
	die_nolog("Usage: %s [-O <stdout>] [-E <stderr>] [-v] config_file\n", progname);
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

// Checks for root AND allows running as a setuid binary!
void ensure_root() {
	if (setuid(0) != 0)
		die_errno("Could not get root privileges");
}

int main(int argc, char *argv[]) {
	read_options(argc, argv);
	start_logging();
	ensure_root();
	start_shared_memory();
	jail_conf = parse_config(filename);
	jail_path = "/usr/jails/base/10.2-RELEASE"; // XXX: mkdtemp(copy_string(TMP_TEMPLATE));
	pid_t child_pid = pdfork(&child_pd, 0);
	if (child_pid == -1)
		die_errno("Could not fork");
	if (child_pid > 0) { // Parent
		sem_wait(jail_started);
		start_signal_handlers();
		if (*jail_id != -1) {
			wait_for_child();
			stop_jail();
		}
		munmap(jail_id, sizeof(*jail_id));
	} else { // Child
		start_jail();
		start_process();
	}
	return 0;
}

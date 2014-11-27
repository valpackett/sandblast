// Sandblast -- the missing simple container tool for FreeBSD
// Copyright (c) 2014 Greg V <greg@unrelenting.technology>
// Licensed under the ISC license, see the COPYING file

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/jail.h>
#include <sys/wait.h>
#include <jansson.h>
#include "util.c"

#define TMP_TEMPLATE "/tmp/sandblast.XXXXXXXX"
#define DEFAULT_PLUGIN_PATH "/usr/local/share/sandblast/plugins"

// Process communication
static sem_t *jail_started;
static sem_t *started_plugins;
static pid_t child_pid;

// Jail configuration and state -- fully known after jail_started fires
// Accessed from the parent process; jail_id is set from the child
static const char *jail_path;
static const char *jail_ip;
static const char *jail_hostname;
static const char *jail_jailname;
static const char *jail_process;
static int *jail_id;
static char *redir_stdout = "/dev/stdout";
static char *redir_stderr = "/dev/stderr";

typedef struct {
	char *name;
	char **env;
	size_t env_len;
} plugin_t;

static plugin_t *jail_plugins;
static size_t jail_plugins_count;
static char *plugin_path = DEFAULT_PLUGIN_PATH;
static char *filename;

static const char *progname;

////////////////////////////////////////////////////////////////////////

void start_jail() {
	struct jail j;
	j.version = JAIL_API_VERSION;
	j.path = jail_path;
	j.hostname = jail_hostname;
	j.jailname = jail_jailname;
	j.ip4s = j.ip6s = 0;
	if (jail_ip != NULL) {
		j.ip4s++;
		j.ip4 = malloc(sizeof(struct in_addr) * j.ip4s);
		j.ip4[0] = parse_ipv4(jail_ip);
	}
	freopen(redir_stdout, "w", stdout);
	freopen(redir_stderr, "w", stderr);
	*jail_id = jail(&j);
	sem_post(jail_started);
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
	if (write(scriptfd, jail_process, strlen(jail_process)) == -1)
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

void execute_plugin(const plugin_t *plugin, const char *arg) {
	char *path = r_asprintf("%s/%s", plugin_path, plugin->name);
	pid_t plugin_pid = fork();
	if (plugin_pid == -1) die_errno("Could not fork for executing plugin %s %s", path, arg);
	if (plugin_pid <= 0) { // Child
		char **env = malloc((plugin->env_len + 9) * sizeof(char*));
		memcpy(env, plugin->env, sizeof(char*) * plugin->env_len);
		size_t i = plugin->env_len;
		env[i++] = r_asprintf("SANDBLAST_PATH=%s", jail_path);
		env[i++] = r_asprintf("SANDBLAST_IPV4=%s", jail_ip);
		env[i++] = r_asprintf("SANDBLAST_HOSTNAME=%s", jail_hostname);
		env[i++] = r_asprintf("SANDBLAST_JAILNAME=%s", jail_jailname);
		env[i++] = r_asprintf("SANDBLAST_JID=%d", *jail_id);
		env[i++] = "PATH=/usr/local/bin:/usr/local/sbin:/usr/games:/usr/bin:/usr/sbin:/bin:/sbin";
		env[i++] = "LC_ALL=en_US.UTF-8";
		env[i++] = "LANG=en_US.UTF-8";
		env[i++] = 0;
		if (execve(path, (char *[]){ path, arg, 0 }, env) == -1)
			die("Could not execute plugin %s", path);
	} else {
		int status; waitpid(plugin_pid, &status, 0);
		info("Plugin %s %s exited with status %d", path, arg, status);
		free(path);
	}
}

void start_plugins() {
	for (size_t i = 0; i < jail_plugins_count; i++)
		execute_plugin(&jail_plugins[i], "start");
	sem_post(started_plugins);
}

void stop_plugins() {
	for (size_t i = jail_plugins_count; i-- ;)
		execute_plugin(&jail_plugins[i], "stop");
}

void handle_sigint() {
	kill(child_pid, SIGTERM);
}

void handle_sigterm() {
	kill(child_pid, SIGKILL);
}

void start_signal_handlers() {
	signal(SIGQUIT, handle_sigint);
	signal(SIGHUP,  handle_sigint);
	signal(SIGINT,  handle_sigint);
	signal(SIGTERM, handle_sigterm);
}

void wait_for_child() {
	setproctitle("[parent of jail %s (JID %d)]", jail_jailname, *jail_id);
	int status; waitpid(child_pid, &status, 0);
	info("Jailed process exited with status %d", status);
}

void start_shared_memory() {
	jail_id = init_shm_int();
	jail_started = init_shm_semaphore();
	started_plugins = init_shm_semaphore();
}

void usage() {
	die("Usage: %s [-p <plugins-path>] [-O <stdout>] [-E <stderr>] file.json", progname);
}

void read_options(int argc, char *argv[]) {
	progname = argv[0];
	int c;
	while ((c = getopt(argc, argv, "p:O:E:?h")) != -1) {
		switch (c) {
			case 'p': plugin_path       = optarg; break;
			case 'O': redir_stdout      = optarg; break;
			case 'E': redir_stderr      = optarg; break;
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

void read_file() {
	json_error_t error;
	json_t *root = json_load_file(filename, 0, &error);
	if (!root || !json_is_object(root))
		die("Incorrect JSON at %s @ %d:%d: %s", error.source, error.line, error.column, error.text);
	str_copy_from_json(jail_hostname, root, "hostname");
	str_copy_from_json(jail_ip, root, "ipv4");
	str_copy_from_json(jail_process, root, "process");
	json_t *plugins; arr_from_json(plugins, root, "plugins");
	jail_plugins_count = json_array_size(plugins);
	jail_plugins = malloc(jail_plugins_count * sizeof(plugin_t));
	size_t i; json_t *item; json_array_foreach(plugins, i, item) {
		if (json_is_object(item)) {
			str_copy_from_json(jail_plugins[i].name, item, "name");
			json_t *options = json_object_get(item, "options");
			if (json_is_object(options)) {
				jail_plugins[i].env_len = json_object_size(options);
				jail_plugins[i].env = malloc(sizeof(char*) * jail_plugins[i].env_len);
				size_t j = 0; const char *key; json_t *val; json_object_foreach(options, key, val) {
					json_assert_string(val, "<plugin option value>");
					char *envkey = copy_uppercase_and_underscore(key);
					char *envval = copy_escape_quotes(json_string_value(val));
					char *envstring = r_asprintf("%s=%s", envkey, envval);
					free(envkey);
					free(envval);
					jail_plugins[i].env[j++] = envstring;
				}
			}
		} else if (json_is_string(item)) {
			jail_plugins[i].name = copy_string(json_string_value(item));
			jail_plugins[i].env_len = 0;
		}
	}

	jail_path = mkdtemp(copy_string(TMP_TEMPLATE));
	if (jail_jailname == NULL)
		jail_jailname = hostname_to_jailname(jail_hostname);
	json_decref(root); // this is why everything is copied
}

void start_logging() {
	openlog(progname, LOG_PID | LOG_PERROR | LOG_CONS, LOG_USER);
	setlogmask(LOG_UPTO(LOG_INFO));
}

// Checks for root AND allows running as a setuid binary!
void ensure_root() {
	if (setuid(0) != 0)
		die_errno("Could not get root privileges");
}

int main(int argc, char *argv[]) {
	start_logging();
	ensure_root();
	start_shared_memory();
	read_options(argc, argv);
	read_file();
	child_pid = fork();
	if (child_pid == -1)
		die_errno("Could not fork");
	if (child_pid > 0) { // Parent
		sem_wait(jail_started);
		start_signal_handlers();
		if (*jail_id != -1) {
			start_plugins();
			wait_for_child();
			stop_jail();
			stop_plugins();
		}
		munmap(jail_id, sizeof(*jail_id));
	} else { // Child
		start_jail();
		sem_wait(started_plugins);
		start_process();
	}
	return 0;
}

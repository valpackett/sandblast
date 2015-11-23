#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include "admin.h"
#include "logging.h"

void do_nothing() {}

void run_process(const char *path, char *const argv[], bool must_die, void (*on_fail)()) {
	pid_t child_pid = fork();
	if (child_pid == -1) {
		on_fail();
		if (must_die)
			die_errno("Could not fork for the %s process", path);
	}
	if (child_pid <= 0) { // Child
		if (execve(path, argv,
				(char *[]){ "PATH=/usr/local/bin:/usr/local/sbin:/usr/games:/usr/bin:/usr/sbin:/bin:/sbin",
				            "LC_ALL=en_US.UTF-8",
				            "LANG=en_US.UTF-8", 0 }) == -1) {
			on_fail();
			if (must_die)
				die_errno("Could not spawn the %s process", path);
		}
	} else { // Parent
		int status; waitpid(child_pid, &status, 0);
		if (status != 0) {
			on_fail();
			if (must_die)
				die("The %s process exited unsuccessfully", path);
		}
	}
}

void rctl(const char *operation, const char *rule) {
	run_process("/usr/bin/rctl", (char *[]){ "/usr/bin/rctl", operation, rule, 0 }, true, do_nothing);
}

void mount_devfs(const char *to, int16_t devfs_ruleset, void (*on_fail)()) {
	run_process("/sbin/mount", (char *[]){ "/sbin/mount", "-t", "devfs", "devfs", to, 0 }, true, on_fail);
	char *devfs_ruleset_string; asprintf(&devfs_ruleset_string, "%d", devfs_ruleset);
	run_process("/sbin/devfs", (char *[]){ "/sbin/devfs", "-m", to, "ruleset", devfs_ruleset_string, 0 }, true, on_fail);
	free(devfs_ruleset_string);
	run_process("/sbin/devfs", (char *[]){ "/sbin/devfs", "-m", to, "rule", "applyset", 0 }, true, on_fail);
}

void mount_nullfs(const char *to, const char *from, bool readonly, void (*on_fail)()) {
	run_process("/sbin/mount_nullfs", (char *[]){ "/sbin/mount_nullfs", "-o", "noatime", "-o", (readonly ? "ro" : "rw"), from, to, 0 }, true, on_fail);
}

void mount_unionfs(const char *to, const char *from, bool readonly, void (*on_fail)()) {
	run_process("/sbin/mount_unionfs", (char *[]){ "/sbin/mount_unionfs", "-o", "noatime", "-o", (readonly ? "ro" : "rw"), from, to, 0 }, true, on_fail);
}

void mkdirp(const char *pathname) {
	run_process("/bin/mkdir", (char *[]){ "/bin/mkdir", "-p", pathname, 0 }, false, do_nothing);
}

void umount(const char *mountpoint) {
	run_process("/sbin/umount", (char *[]){ "/sbin/umount", "-f", mountpoint, 0 }, false, do_nothing);
}

void ifconfig_alias(const char *interface, const char *net, const char *address) {
	run_process("/sbin/ifconfig", (char *[]){ "/sbin/ifconfig", interface, net, address, "alias", 0 }, true, do_nothing);
}

void ifconfig_unalias(const char *interface, const char *net, const char *address) {
	run_process("/sbin/ifconfig", (char *[]){ "/sbin/ifconfig", interface, net, address, "-alias", 0 }, true, do_nothing);
}

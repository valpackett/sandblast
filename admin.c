#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include "admin.h"
#include "logging.h"

void run_process(const char *path, char *const argv[]) {
	pid_t child_pid = fork();
	if (child_pid == -1)
		die_errno("Could not fork for the %s process", path);
	if (child_pid <= 0) { // Child
		if (execve(path, argv,
				(char *[]){ "PATH=/usr/local/bin:/usr/local/sbin:/usr/games:/usr/bin:/usr/sbin:/bin:/sbin",
				            "LC_ALL=en_US.UTF-8",
				            "LANG=en_US.UTF-8", 0 }) == -1)
			die_errno("Could not spawn the %s process", path);
	} else { // Parent
		int status; waitpid(child_pid, &status, 0);
		if (status != 0)
			die("The %s process exited unsuccessfully", path);
	}
}

void rctl(const char *operation, const char *rule) {
	run_process("/usr/bin/rctl", (char *[]){ "/usr/bin/rctl", operation, rule, 0 });
}

void mount_nullfs(const char *to, const char *from, bool readonly) {
	run_process("/sbin/mount_nullfs", (char *[]){ "/sbin/mount_nullfs", "-o", (readonly ? "ro" : "rw"), from, to, 0 });
}

void mkdirp(const char *pathname) {
	run_process("/bin/mkdir", (char *[]){ "/bin/mkdir", "-p", pathname, 0 });
}

void umount(const char *mountpoint) {
	run_process("/sbin/umount", (char *[]){ "/sbin/umount", mountpoint, 0 });
}

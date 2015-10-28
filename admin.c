#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "admin.h"
#include "logging.h"

void rctl(const char *operation, const char *rule) {
	pid_t child_pid = fork();
	if (child_pid == -1)
		die_errno("Could not fork");
	if (child_pid <= 0) { // Child
		if (execve("/usr/bin/rctl", (char *[]){ "/usr/bin/rctl", operation, rule, 0 },
				(char *[]){ "PATH=/usr/local/bin:/usr/local/sbin:/usr/games:/usr/bin:/usr/sbin:/bin:/sbin",
				            "LC_ALL=en_US.UTF-8",
				            "LANG=en_US.UTF-8", 0 }) == -1)
			die_errno("Could not spawn the rctl process");
	} else { // Parent
		int status; waitpid(child_pid, &status, 0);
		if (status != 0)
			die("The rctl process exited unsuccessfully");
	}
}

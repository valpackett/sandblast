#pragma once

#define RCTL_ADD "-a"
#define RCTL_REMOVE "-r"

void rctl(const char *operation, const char *rule);
void mount_nullfs(const char *to, const char *from, bool readonly);

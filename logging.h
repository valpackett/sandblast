#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>

char *r_asprintf(const char *fmt, ...);

void s_log(const int priority, const char *fmt, ...);

void s_log_errno(const int priority, const char *fmt, ...);

#define info(...) if (1) { s_log(LOG_INFO, __VA_ARGS__, NULL); }
#define die(...) if (1) { s_log(LOG_ERR, __VA_ARGS__, NULL); exit(1); }
#define die_errno(...) if (1) { s_log_errno(LOG_ERR, __VA_ARGS__, NULL); exit(errno); }
#define die_nolog(...) if (1) { printf(__VA_ARGS__); exit(1); }

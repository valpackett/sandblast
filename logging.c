#include <string.h>
#include "logging.h"

char *r_asprintf(const char *fmt, ...) {
	va_list vargs; va_start(vargs, fmt);
	char *result;
	vasprintf(&result, fmt, vargs);
	va_end(vargs);
	return result;
}

void s_log(const int priority, const char *fmt, ...) {
	va_list vargs; va_start(vargs, fmt);
	vsyslog(priority, fmt, vargs);
	va_end(vargs);
}

void s_log_errno(const int priority, const char *fmt, ...) {
	va_list vargs; va_start(vargs, fmt);
	char *e = r_asprintf("errno %d (%s)", errno, strerror(errno));
	char *m; vasprintf(&m, fmt, vargs); 
	va_end(vargs);
	s_log(priority, "%s: %s", e, m, NULL);
}

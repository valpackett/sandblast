// Sandblast -- the missing simple container tool for FreeBSD
// Copyright (c) 2014-2015 Greg V <greg@unrelenting.technology>
// Available under the ISC license, see the COPYING file

#ifndef _SANDBLAST_UTIL
#define _SANDBLAST_UTIL

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdbool.h>
#include <jansson.h>
#include <jail.h>

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

#define json_assert_string(thing, name) \
	if (!json_is_string(thing)) \
		die("Incorrect JSON: %s must be a string", name); \

#define str_copy_from_json_optional(target, parent, name) \
	if (1) { \
		json_t *result = json_object_get(parent, name); \
		if (result != NULL) { \
			json_assert_string(result, name); \
			target = copy_string(json_string_value(result)); \
		} \
	}

#define str_copy_from_json(target, parent, name) \
	if (1) { \
		json_t *result = json_object_get(parent, name); \
		json_assert_string(result, name); \
		target = copy_string(json_string_value(result)); \
	}

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

#define info(...) if (1) { s_log(LOG_INFO, __VA_ARGS__, NULL); }
#define die(...) if (1) { s_log(LOG_ERR, __VA_ARGS__, NULL); exit(1); }
#define die_errno(...) if (1) { s_log_errno(LOG_ERR, __VA_ARGS__, NULL); exit(errno); }
#define die_nolog(...) if (1) { printf(__VA_ARGS__); exit(1); }

sem_t *init_shm_semaphore() {
	sem_t *result = (sem_t*) mmap(NULL, sizeof(*result), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (result == MAP_FAILED)
		die_errno("Could not mmap");
	sem_init(result, 1, 0);
	return result;
}

int *init_shm_int() {
	int *result = (int*) mmap(NULL, sizeof(*result), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (result == MAP_FAILED)
		die_errno("Could not mmap");
	*result = -1;
	return result;
}

char *copy_string(const char *original) {
	size_t len = strlen(original) + 1;
	char *result = malloc(len);
	strlcpy(result, original, len);
	return result;
}

// For names of environment variables
char *copy_uppercase_and_underscore(const char *original) {
	size_t len = strlen(original) + 1;
	char *result = malloc(len);
	for (size_t i = 0; i < len; i++) {
		if (original[i] >= 'a' && original[i] <= 'z')
			result[i] = original[i] - 32;
		else if (original[i] >= 'A' && original[i] <= 'Z')
			result[i] = original[i];
		else
			result[i] = '_';
	}
	result[len - 1] = '\0';
	return result;
}

// The jailname can't contain periods, unlike hostname, which often should
char *hostname_to_jailname(const char *original) {
	size_t len = strlen(original) + 1;
	char *result = malloc(len);
	for (size_t i = 0; i < len; i++)
		result[i] = (original[i] == '.') ? '_' : original[i];
	result[len - 1] = '\0';
	return result;
}

// str.replace('"', '\\"')
char* copy_escape_quotes(const char *original) {
	size_t qcount = 0;
	size_t len = strlen(original) + 1;
	for (size_t i = 0; i < len; i++)
		if (original[i] == '"')
			qcount++;
	char* result = malloc(len + qcount + 1);
	size_t r = 0;
	for (size_t i = 0; i < len; i++) {
		if (original[i] == '"') {
			result[r] = '\\';
			r++;
		}
		result[r] = original[i];
		r++;
	}
	result[r] = '\0';
	return result;
}

#endif

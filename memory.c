#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "logging.h"
#include "memory.h"

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

void *init_shm_arena(size_t size) {
	void *arena = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (arena == MAP_FAILED)
		die_errno("Could not mmap");
	size_t *arena_size = (size_t*)arena;
	*arena_size = size;
	size_t *arena_cur = (size_t*)(arena + sizeof(size_t));
	*arena_cur = 2*sizeof(size_t);
	return arena;
}

void *arena_alloc(const void *arena, size_t size) {
	size_t *arena_size = (size_t*)arena;
	size_t *arena_cur = (size_t*)(arena + sizeof(size_t));
	*arena_cur += size;
	if (*arena_cur >= *arena_size)
		die("Too much data");
	bzero(arena + *arena_cur - size, size);
	return arena + *arena_cur - size;
}

char *copy_string(const char *original) {
	size_t len = strlen(original) + 1;
	char *result = malloc(len);
	if (strlcpy(result, original, len) >= len)
		die("String copying error");
	return result;
}

char *join_strings(const char**srcs, size_t srcs_len, char sep) {
	size_t len = 1;
	for (size_t i = 0; i < srcs_len; i++)
		if (srcs[i] != NULL)
			len += strlen(srcs[i]) + 1;
	char *result = (char*)malloc(len);
	bzero(result, len);
	size_t pos = 0;
	for (size_t i = 0; i < srcs_len; i++) {
		if (srcs[i] == NULL) continue;
		pos = strlcat(result, srcs[i], len);
		if (pos >= len)
			die("String join error");
		result[pos] = sep;
	}
	result[pos] = '\0';
	return result;
}

// NOTE: Does not free because it's used with strings managed by libucl
void deduplicate_strings(const char **strs, size_t strs_len) {
	for (size_t i = 0; i < strs_len; i++) {
		if (strs[i] == NULL) continue;
		for (size_t j = 0; j < i; j++) {
			if (strs[j] == NULL) continue;
			if (strcmp(strs[i], strs[j]) == 0)
				strs[j] = NULL;
		}
	}
}

char *ipaddr_string(const char **addrs, size_t addrs_len) {
	deduplicate_strings(addrs, addrs_len);
	return join_strings(addrs, addrs_len, ',');
}

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
	return arena + *arena_cur - size;
}

char *copy_string(const char *original) {
	size_t len = strlen(original) + 1;
	char *result = malloc(len);
	strlcpy(result, original, len);
	return result;
}

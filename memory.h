#pragma once

#include <semaphore.h>

sem_t *init_shm_semaphore();

int *init_shm_int();

void *init_shm_arena(size_t size);

void *arena_alloc(const void *arena, size_t size);

char *copy_string(const char *original);

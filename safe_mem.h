#ifndef SAFE_MEMORY_H
#define SAFE_MEMORY_H

#include <stdio.h>
#include <stdlib.h>

static inline void *safe_malloc(size_t size) {
  void *alloc_mem = malloc(size);
  if (!alloc_mem && size > 0) {
    fprintf(stderr, "Error: Memory allocation failed (size: %zu bytes)\n",
            size);
    exit(EXIT_FAILURE);
  }
  return alloc_mem;
}

static inline void *safe_calloc(size_t num, size_t size) {
  void *alloc_mem = calloc(num, size);
  if (!alloc_mem && num > 0 && size > 0) {
    fprintf(stderr,
            "Error: Memory allocation failed (num: %zu, size: %zu bytes)\n",
            num, size);
    exit(EXIT_FAILURE);
  }
  return alloc_mem;
}

static inline void *safe_realloc(void *ptr, size_t new_size) {
  void *alloc_mem = realloc(ptr, new_size);
  if (!alloc_mem && new_size > 0) {
    fprintf(stderr, "Error: Memory reallocation failed (new size: %zu bytes)\n",
            new_size);
    exit(EXIT_FAILURE);
  }
  return alloc_mem;
}

#endif // SAFE_MEMORY_H

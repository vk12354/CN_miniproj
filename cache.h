#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>

typedef enum {
    CACHE_MISS = 0,
    CACHE_HIT = 1
} CacheResult;

CacheResult cache_lookup(const char *key, char **data, size_t *size, int ttl_seconds);
int cache_store(const char *key, const char *data, size_t size);
void cache_init(const char *directory);

#endif

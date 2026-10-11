#include "cache.h"
#include "proxy.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char cache_dir[512] = "cache";
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    uint64_t magic;
    int64_t timestamp;
    uint64_t key_len;
    uint64_t data_len;
} CacheHeader;

static uint64_t fnv1a64(const char *s) {
    uint64_t h = UINT64_C(1469598103934665603);
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= UINT64_C(1099511628211);
    }
    return h;
}

static void cache_path(const char *key, char *path, size_t n) {
    snprintf(path, n, "%s/%016llx.cache", cache_dir,
             (unsigned long long)fnv1a64(key));
}

void cache_init(const char *directory) {
    if (directory && *directory)
        snprintf(cache_dir, sizeof(cache_dir), "%s", directory);
    mkdir(cache_dir, 0755);
}

CacheResult cache_lookup(const char *key, char **data, size_t *size, int ttl_seconds) {
    *data = NULL;
    *size = 0;

    char path[1024];
    cache_path(key, path, sizeof(path));

    pthread_mutex_lock(&cache_mutex);
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        pthread_mutex_unlock(&cache_mutex);
        return CACHE_MISS;
    }

    CacheHeader h;
    if (fread(&h, sizeof(h), 1, fp) != 1 || h.magic != CACHE_MAGIC ||
        h.key_len > 4096 || h.data_len > MAX_CACHE_RESPONSE) {
        fclose(fp);
        unlink(path);
        pthread_mutex_unlock(&cache_mutex);
        return CACHE_MISS;
    }

    char *stored_key = malloc((size_t)h.key_len + 1);
    if (!stored_key) {
        fclose(fp);
        pthread_mutex_unlock(&cache_mutex);
        return CACHE_MISS;
    }
    if (fread(stored_key, 1, (size_t)h.key_len, fp) != h.key_len) {
        free(stored_key);
        fclose(fp);
        pthread_mutex_unlock(&cache_mutex);
        return CACHE_MISS;
    }
    stored_key[h.key_len] = '\0';

    time_t now = time(NULL);
    if (strcmp(stored_key, key) != 0 || h.timestamp <= 0 ||
        now - (time_t)h.timestamp > ttl_seconds) {
        free(stored_key);
        fclose(fp);
        unlink(path);
        pthread_mutex_unlock(&cache_mutex);
        return CACHE_MISS;
    }
    free(stored_key);

    char *buf = malloc((size_t)h.data_len);
    if (!buf) {
        fclose(fp);
        pthread_mutex_unlock(&cache_mutex);
        return CACHE_MISS;
    }
    if (fread(buf, 1, (size_t)h.data_len, fp) != h.data_len) {
        free(buf);
        fclose(fp);
        pthread_mutex_unlock(&cache_mutex);
        return CACHE_MISS;
    }

    fclose(fp);
    pthread_mutex_unlock(&cache_mutex);
    *data = buf;
    *size = (size_t)h.data_len;
    return CACHE_HIT;
}

int cache_store(const char *key, const char *data, size_t size) {
    if (!key || !data || size == 0 || size > MAX_CACHE_RESPONSE) return 0;

    char path[1024], tmp_path[1080];
    cache_path(key, path, sizeof(path));
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%ld", path, (long)getpid());

    CacheHeader h = {
        .magic = CACHE_MAGIC,
        .timestamp = time(NULL),
        .key_len = strlen(key),
        .data_len = size
    };

    pthread_mutex_lock(&cache_mutex);
    FILE *fp = fopen(tmp_path, "wb");
    if (!fp) {
        pthread_mutex_unlock(&cache_mutex);
        return -1;
    }

    int ok = fwrite(&h, sizeof(h), 1, fp) == 1 &&
             fwrite(key, 1, (size_t)h.key_len, fp) == h.key_len &&
             fwrite(data, 1, size, fp) == size;

    if (fclose(fp) != 0) ok = 0;
    if (ok && rename(tmp_path, path) != 0) ok = 0;
    if (!ok) unlink(tmp_path);

    pthread_mutex_unlock(&cache_mutex);
    return ok ? 0 : -1;
}

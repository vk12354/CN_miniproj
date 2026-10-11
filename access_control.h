#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#include <stddef.h>

typedef struct {
    char **domains;
    size_t count;
} Blocklist;

int blocklist_load(const char *path, Blocklist *list);
int blocklist_contains(const Blocklist *list, const char *host);
void blocklist_free(Blocklist *list);

#endif

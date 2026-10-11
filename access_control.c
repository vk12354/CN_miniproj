#include "access_control.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static char *trim_ws(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return s;
}

int blocklist_load(const char *path, Blocklist *list) {
    memset(list, 0, sizeof(*list));
    list->domains = calloc(1024, sizeof(*list->domains));
    if (!list->domains) return -1;
    FILE *fp = fopen(path, "r");
    if (!fp) {
        free(list->domains);
        list->domains = NULL;
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *s = trim_ws(line);
        if (*s == '\0' || *s == '#') continue;
        if (list->count >= 1024) break;
        size_t n = strlen(s);
        if (n && s[n - 1] == '\n') s[n - 1] = '\0';
        char *copy = strdup(s);
        if (!copy) {
            fclose(fp);
            blocklist_free(list);
            return -1;
        }
        list->domains[list->count++] = copy;
    }
    fclose(fp);
    return 0;
}

/* Lazy allocation of the pointer array keeps the struct small. */
int blocklist_contains(const Blocklist *list, const char *host) {
    if (!list || !host) return 0;
    for (size_t i = 0; i < list->count; i++) {
        if (strcasecmp(list->domains[i], host) == 0) return 1;
        size_t hlen = strlen(host), dlen = strlen(list->domains[i]);
        if (hlen > dlen && host[hlen - dlen - 1] == '.' &&
            strcasecmp(host + hlen - dlen, list->domains[i]) == 0) return 1;
    }
    return 0;
}

void blocklist_free(Blocklist *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) free(list->domains[i]);
    free(list->domains);
    list->domains = NULL;
    list->count = 0;
}

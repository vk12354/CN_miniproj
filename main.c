#include "access_control.h"
#include "cache.h"
#include "logger.h"
#include "network.h"
#include "server.h"
#include "proxy.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int proxy_fd = -1;
static Blocklist blocklist;

static void handle_signal(int sig) {
    (void)sig;
    close_fd(&proxy_fd);
    _exit(0);
}

int main(int argc, char **argv) {
    int port = DEFAULT_PROXY_PORT;
    if (argc > 1) {
        char *end = NULL;
        long p = strtol(argv[1], &end, 10);
        if (!end || *end || p < 1 || p > 65535) {
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            return 1;
        }
        port = (int)p;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    mkdir("cache", 0755);
    mkdir("logs", 0755);

    if (blocklist_load("config/blocklist.txt", &blocklist) < 0) {
        fprintf(stderr, "Warning: could not load config/blocklist.txt\n");
        blocklist.domains = calloc(1, sizeof(char *));
        blocklist.count = 0;
    }
    cache_init("cache");
    if (logger_init("logs/proxy.log") < 0) {
        perror("logs/proxy.log");
        blocklist_free(&blocklist);
        return 1;
    }

    proxy_fd = create_listener(port, 128);
    if (proxy_fd < 0) {
        perror("create_listener");
        logger_close();
        blocklist_free(&blocklist);
        return 1;
    }

    printf("Proxy server listening on 0.0.0.0:%d\n", port);
    printf("Cache TTL: %d seconds\n", CACHE_TTL_SECONDS);
    printf("Press Ctrl+C to stop.\n");
    fflush(stdout);

    int rc = run_server(proxy_fd, &blocklist);
    close_fd(&proxy_fd);
    logger_close();
    blocklist_free(&blocklist);
    return rc == 0 ? 0 : 1;
}

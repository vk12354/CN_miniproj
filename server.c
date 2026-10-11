#include "server.h"
#include "cache.h"
#include "forwarder.h"
#include "http_parser.h"
#include "logger.h"
#include "network.h"
#include "proxy.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int client_fd;
    struct sockaddr_storage addr;
    socklen_t addr_len;
    const Blocklist *blocklist;
} ClientContext;

static void client_ip(const struct sockaddr_storage *addr, char *out, size_t out_size) {
    const void *src = NULL;
    if (addr->ss_family == AF_INET) src = &((const struct sockaddr_in *)addr)->sin_addr;
    else if (addr->ss_family == AF_INET6) src = &((const struct sockaddr_in6 *)addr)->sin6_addr;
    if (!src || !inet_ntop(addr->ss_family, src, out, out_size)) snprintf(out, out_size, "unknown");
}

static void send_simple_error(int fd, int status, const char *reason, const char *message) {
    char response[512];
    int n = snprintf(response, sizeof(response),
                     "HTTP/1.1 %d %s\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
                     status, reason, strlen(message), message);
    if (n > 0) send_all(fd, response, (size_t)n);
}

static long elapsed_ms(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000L + (end->tv_nsec - start->tv_nsec) / 1000000L;
}

static void *client_thread(void *arg) {
    ClientContext *ctx = arg;
    int client_fd = ctx->client_fd;
    const Blocklist *blocklist = ctx->blocklist;
    char ip[INET6_ADDRSTRLEN];
    client_ip(&ctx->addr, ip, sizeof(ip));
    free(ctx);

    char raw[MAX_REQUEST_SIZE];
    size_t raw_len = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int rr = read_http_request(client_fd, raw, sizeof(raw), &raw_len);
    if (rr != 0) {
        send_simple_error(client_fd, 400, "Bad Request", "Malformed or oversized HTTP request\n");
        clock_gettime(CLOCK_MONOTONIC, &end);
        logger_log(ip, "-", "-", "BAD_REQUEST", 400, 0, elapsed_ms(&start, &end));
        close_fd(&client_fd);
        return NULL;
    }

    char raw_copy[MAX_REQUEST_SIZE];
    memcpy(raw_copy, raw, raw_len + 1);
    HttpRequest req;
    int pr = parse_http_request(raw_copy, raw_len, &req);
    if (pr < 0) {
        send_simple_error(client_fd, 400, "Bad Request", "Invalid HTTP request\n");
        clock_gettime(CLOCK_MONOTONIC, &end);
        logger_log(ip, "-", "-", "BAD_REQUEST", 400, 0, elapsed_ms(&start, &end));
        close_fd(&client_fd);
        return NULL;
    }
    if (pr > 0) {
        send_simple_error(client_fd, 501, "Not Implemented", "Only GET is supported\n");
        clock_gettime(CLOCK_MONOTONIC, &end);
        logger_log(ip, req.method, req.path, "NOT_IMPLEMENTED", 501, 0, elapsed_ms(&start, &end));
        close_fd(&client_fd);
        return NULL;
    }

    char url_key[8192];
    if (req.port == 80) snprintf(url_key, sizeof(url_key), "%s%s", req.host, req.path);
    else snprintf(url_key, sizeof(url_key), "%s:%d%s", req.host, req.port, req.path);

    if (blocklist_contains(blocklist, req.host)) {
        send_simple_error(client_fd, 403, "Forbidden", "Blocked by proxy access-control policy\n");
        clock_gettime(CLOCK_MONOTONIC, &end);
        logger_log(ip, req.method, url_key, "BLOCKED", 403, 0, elapsed_ms(&start, &end));
        close_fd(&client_fd);
        return NULL;
    }

    if (strcasecmp(req.method, "GET") == 0) {
        char *cached = NULL;
        size_t cached_size = 0;
        if (cache_lookup(url_key, &cached, &cached_size, CACHE_TTL_SECONDS) == CACHE_HIT) {
            int ok = send_all(client_fd, cached, cached_size);
            free(cached);
            clock_gettime(CLOCK_MONOTONIC, &end);
            logger_log(ip, req.method, url_key, ok == 0 ? "HIT" : "SEND_ERROR",
                       ok == 0 ? 200 : 0, cached_size, elapsed_ms(&start, &end));
            close_fd(&client_fd);
            return NULL;
        }
    }

    char origin_request[MAX_REQUEST_SIZE * 2];
    size_t origin_len = 0;
    char raw_for_build[MAX_REQUEST_SIZE];
    memcpy(raw_for_build, raw, raw_len + 1);
    if (build_origin_request(raw_for_build, raw_len, &req, origin_request, sizeof(origin_request), &origin_len) < 0) {
        send_simple_error(client_fd, 400, "Bad Request", "Could not normalize HTTP request\n");
        clock_gettime(CLOCK_MONOTONIC, &end);
        logger_log(ip, req.method, url_key, "BAD_REQUEST", 400, 0, elapsed_ms(&start, &end));
        close_fd(&client_fd);
        return NULL;
    }

    char *captured = NULL;
    size_t captured_size = 0;
    int status = 0;
    int fr = forward_request(client_fd, &req, origin_request, origin_len,
                             &captured, &captured_size, &status);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long latency = elapsed_ms(&start, &end);
    if (fr == -2) {
        send_simple_error(client_fd, 502, "Bad Gateway", "Destination server unavailable\n");
        logger_log(ip, req.method, url_key, "CONNECT_FAIL", 502, 0, latency);
    } else if (fr != 0) {
        logger_log(ip, req.method, url_key, "FORWARD_ERROR", status ? status : 0, 0, latency);
    } else {
        if (strcasecmp(req.method, "GET") == 0 && status == 200 && captured && captured_size > 0) {
            cache_store(url_key, captured, captured_size);
        }
        logger_log(ip, req.method, url_key, "MISS", status, captured_size, latency);
    }

    free(captured);
    close_fd(&client_fd);
    return NULL;
}

int run_server(int listen_fd, const Blocklist *blocklist) {
    for (;;) {
        ClientContext *ctx = calloc(1, sizeof(*ctx));
        if (!ctx) {
            fprintf(stderr, "Out of memory while accepting client\n");
            continue;
        }
        ctx->addr_len = sizeof(ctx->addr);
        ctx->client_fd = accept(listen_fd, (struct sockaddr *)&ctx->addr, &ctx->addr_len);
        if (ctx->client_fd < 0) {
            free(ctx);
            if (errno == EINTR) continue;
            perror("accept");
            return -1;
        }
        if (set_socket_timeouts(ctx->client_fd, DEFAULT_CLIENT_TIMEOUT) < 0) {
            close(ctx->client_fd);
            free(ctx);
            continue;
        }
        ctx->blocklist = blocklist;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, ctx) != 0) {
            close(ctx->client_fd);
            free(ctx);
            continue;
        }
        pthread_detach(tid);
    }
}

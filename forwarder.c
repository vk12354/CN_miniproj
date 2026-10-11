#include "forwarder.h"
#include "network.h"
#include "proxy.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int append_buffer(char **buf, size_t *size, size_t *cap, const char *data, size_t n) {
    if (*size + n > MAX_CACHE_RESPONSE) return 1; /* too large to cache */
    if (*size + n > *cap) {
        size_t new_cap = *cap ? *cap : 16384;
        while (new_cap < *size + n) {
            if (new_cap > MAX_CACHE_RESPONSE / 2) {
                new_cap = MAX_CACHE_RESPONSE;
                break;
            }
            new_cap *= 2;
        }
        char *tmp = realloc(*buf, new_cap);
        if (!tmp) return 1;
        *buf = tmp;
        *cap = new_cap;
    }
    memcpy(*buf + *size, data, n);
    *size += n;
    return 0;
}

int forward_request(int client_fd, const HttpRequest *request,
                    const char *origin_request, size_t origin_request_len,
                    char **captured_response, size_t *captured_size,
                    int *status_code) {
    *captured_response = NULL;
    *captured_size = 0;
    *status_code = 0;

    int server_fd = connect_to_host(request->host, request->port, DEFAULT_CONNECT_TIMEOUT);
    if (server_fd < 0) return -2;

    if (send_all(server_fd, origin_request, origin_request_len) < 0) {
        close_fd(&server_fd);
        return -1;
    }

    char *cache_buf = NULL;
    size_t cache_size = 0, cache_cap = 0;
    int cacheable_size = 1;
    int parsed_status = 0;
    char response_head[8192];
    size_t head_size = 0;
    int head_done = 0;

    for (;;) {
        char buf[BUFFER_SIZE];
        ssize_t n = recv(server_fd, buf, sizeof(buf), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            free(cache_buf);
            close_fd(&server_fd);
            return -1;
        }
        if (n == 0) break;

        if (send_all(client_fd, buf, (size_t)n) < 0) {
            free(cache_buf);
            close_fd(&server_fd);
            return -1;
        }

        if (!head_done) {
            size_t copy = (size_t)n;
            if (copy > sizeof(response_head) - head_size - 1) copy = sizeof(response_head) - head_size - 1;
            memcpy(response_head + head_size, buf, copy);
            head_size += copy;
            response_head[head_size] = '\0';
            if (strstr(response_head, "\r\n\r\n") || strstr(response_head, "\n\n")) {
                head_done = 1;
                parsed_status = parse_status_code(response_head, head_size);
            }
        }

        if (cacheable_size) {
            if (append_buffer(&cache_buf, &cache_size, &cache_cap, buf, (size_t)n) != 0) {
                cacheable_size = 0;
                free(cache_buf);
                cache_buf = NULL;
                cache_size = 0;
                cache_cap = 0;
            }
        }
    }

    close_fd(&server_fd);
    *status_code = parsed_status;
    if (cacheable_size && cache_buf && cache_size <= MAX_CACHE_RESPONSE) {
        *captured_response = cache_buf;
        *captured_size = cache_size;
    } else {
        free(cache_buf);
    }
    return 0;
}

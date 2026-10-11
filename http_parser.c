#include "http_parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return s;
}

static int parse_host_port(const char *value, char *host, size_t host_size, int *port) {
    char tmp[MAX_HOST_LEN + 16];
    if (strlen(value) >= sizeof(tmp)) return -1;
    strcpy(tmp, value);
    char *p = trim(tmp);

    *port = 80;
    if (*p == '[') {
        char *close = strchr(p, ']');
        if (!close) return -1;
        size_t n = (size_t)(close - p - 1);
        if (n == 0 || n >= host_size) return -1;
        memcpy(host, p + 1, n);
        host[n] = '\0';
        if (close[1] == ':') {
            char *end = NULL;
            long v = strtol(close + 2, &end, 10);
            if (!end || *end || v < 1 || v > 65535) return -1;
            *port = (int)v;
        }
        return 0;
    }

    char *colon = strrchr(p, ':');
    if (colon && strchr(colon + 1, ':') == NULL) {
        char *end = NULL;
        long v = strtol(colon + 1, &end, 10);
        if (end && *end == '\0' && v >= 1 && v <= 65535) {
            *port = (int)v;
            *colon = '\0';
        }
    }

    if (*p == '\0' || strlen(p) >= host_size) return -1;
    strcpy(host, p);
    return 0;
}

int read_http_request(int client_fd, char *buf, size_t buf_size, size_t *request_len) {
    size_t total = 0;
    while (total + 1 < buf_size) {
        ssize_t n = recv(client_fd, buf + total, buf_size - total - 1, 0);
        if (n < 0) return -1;
        if (n == 0) break;
        total += (size_t)n;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n") || strstr(buf, "\n\n")) {
            *request_len = total;
            return 0;
        }
    }
    *request_len = total;
    return total + 1 >= buf_size ? 1 : (total ? 0 : -1);
}

int parse_http_request(char *raw, size_t len, HttpRequest *request) {
    (void)len;
    memset(request, 0, sizeof(*request));
    request->port = 80;

    char *line_end = strstr(raw, "\r\n");
    if (!line_end) line_end = strstr(raw, "\n");
    if (!line_end) return -1;
    *line_end = '\0';

    char *save = NULL;
    char *method = strtok_r(raw, " \t", &save);
    char *target = strtok_r(NULL, " \t", &save);
    char *version = strtok_r(NULL, " \t", &save);
    if (!method || !target || !version) return -1;

    if (strlen(method) > MAX_METHOD_LEN || strlen(version) >= sizeof(request->version)) return -1;
    strcpy(request->method, method);
    strcpy(request->version, version);

    char *headers = line_end + (line_end[0] == '\r' ? 2 : 1);
    char host_header[MAX_HOST_LEN + 16] = {0};
    for (char *line = headers; *line;) {
        char *next = strstr(line, "\r\n");
        size_t step = 2;
        if (!next) {
            next = strchr(line, '\n');
            step = 1;
        }
        if (!next) break;
        if (next == line) break;

        *next = '\0';
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *name = trim(line);
            char *value = trim(colon + 1);
            if (strcasecmp(name, "Host") == 0) {
                if (strlen(value) >= sizeof(host_header)) return -1;
                strcpy(host_header, value);
            }
        }
        *next = '\r';
        line = next + step;
    }

    if (strncasecmp(target, "http://", 7) == 0) {
        char url[MAX_REQUEST_SIZE];
        if (strlen(target) >= sizeof(url)) return -1;
        strcpy(url, target + 7);
        char *slash = strchr(url, '/');
        if (slash) {
            strcpy(request->path, slash);
            *slash = '\0';
        } else {
            strcpy(request->path, "/");
        }
        if (parse_host_port(url, request->host, sizeof(request->host), &request->port) < 0) return -1;
    } else {
        if (target[0] != '/') return -1;
        if (strlen(target) > MAX_PATH_LEN) return -1;
        strcpy(request->path, target);
        if (host_header[0] == '\0') return -1;
        if (parse_host_port(host_header, request->host, sizeof(request->host), &request->port) < 0) return -1;
    }

    if (strcasecmp(request->method, "GET") != 0) return 1;
    return 0;
}

int build_origin_request(const char *raw, size_t len, const HttpRequest *request,
                         char *out, size_t out_size, size_t *out_len) {
    (void)len;
    const char *first_end = strstr(raw, "\r\n");
    if (!first_end) first_end = strstr(raw, "\n");
    if (!first_end) return -1;

    size_t written = 0;
    int n = snprintf(out, out_size, "%s %s HTTP/1.0\r\nHost: %s", request->method,
                     request->path, request->host);
    if (n < 0 || (size_t)n >= out_size) return -1;
    written = (size_t)n;
    if (request->port != 80) {
        n = snprintf(out + written, out_size - written, ":%d", request->port);
        if (n < 0 || (size_t)n >= out_size - written) return -1;
        written += (size_t)n;
    }
    n = snprintf(out + written, out_size - written, "\r\n");
    if (n < 0 || (size_t)n >= out_size - written) return -1;
    written += (size_t)n;

    const char *headers = first_end + (first_end[0] == '\r' ? 2 : 1);
    const char *p = headers;
    while (*p) {
        const char *e = strstr(p, "\r\n");
        size_t step = 2;
        if (!e) {
            e = strchr(p, '\n');
            step = 1;
        }
        if (!e) break;
        if (e == p) break;

        size_t line_len = (size_t)(e - p);
        char line[MAX_REQUEST_SIZE];
        if (line_len >= sizeof(line)) return -1;
        memcpy(line, p, line_len);
        line[line_len] = '\0';
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *name = trim(line);
            if (strcasecmp(name, "Host") != 0 &&
                strcasecmp(name, "Connection") != 0 &&
                strcasecmp(name, "Proxy-Connection") != 0 &&
                strcasecmp(name, "Keep-Alive") != 0) {
                if (written + line_len + 2 >= out_size) return -1;
                memcpy(out + written, p, line_len);
                written += line_len;
                memcpy(out + written, "\r\n", 2);
                written += 2;
            }
        }
        p = e + step;
    }

    n = snprintf(out + written, out_size - written, "Connection: close\r\n\r\n");
    if (n < 0 || (size_t)n >= out_size - written) return -1;
    written += (size_t)n;
    *out_len = written;
    return 0;
}

int parse_status_code(const char *response, size_t len) {
    if (len < 12) return 0;
    const char *sp = strchr(response, ' ');
    if (!sp || sp + 4 > response + len) return 0;
    return atoi(sp + 1);
}

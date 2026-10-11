#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>

#include "proxy.h"

typedef struct {
    char method[MAX_METHOD_LEN + 1];
    char host[MAX_HOST_LEN + 1];
    int port;
    char path[MAX_PATH_LEN + 1];
    char version[16];
} HttpRequest;

int read_http_request(int client_fd, char *buf, size_t buf_size, size_t *request_len);
int parse_http_request(char *raw, size_t len, HttpRequest *request);
int build_origin_request(const char *raw, size_t len, const HttpRequest *request,
                         char *out, size_t out_size, size_t *out_len);
int parse_status_code(const char *response, size_t len);

#endif

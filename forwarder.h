#ifndef FORWARDER_H
#define FORWARDER_H

#include <stddef.h>
#include "http_parser.h"

int forward_request(int client_fd, const HttpRequest *request,
                    const char *origin_request, size_t origin_request_len,
                    char **captured_response, size_t *captured_size,
                    int *status_code);

#endif

#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>

int logger_init(const char *path);
void logger_close(void);
void logger_log(const char *client_ip, const char *method, const char *url,
                const char *result, int status, size_t bytes, long latency_ms);
void logger_log_error(const char *client_ip, const char *message);

#endif

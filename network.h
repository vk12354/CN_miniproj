#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

int create_listener(int port, int backlog);
int connect_to_host(const char *host, int port, int timeout_seconds);
int set_socket_timeouts(int fd, int timeout_seconds);
int send_all(int fd, const void *buf, size_t len);
void close_fd(int *fd);

#endif

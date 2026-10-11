#ifndef SERVER_H
#define SERVER_H

#include "access_control.h"

int run_server(int listen_fd, const Blocklist *blocklist);

#endif

#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int sendall(int s, char *buf, int *len, struct sockaddr* p);

#endif
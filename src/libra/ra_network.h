//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_network.h
//

#ifndef __RA_NETWORK_H__
#define __RA_NETWORK_H__

#include <stddef.h>

typedef struct ra_network *ra_network_t;

typedef void (*ra_network_fnc_t)(void *ctx, ra_network_t network);

int ra_network_listen(const char *hostname,
                      const char *servname,
                      ra_network_fnc_t fnc,
                      void *ctx);

ra_network_t ra_network_connect(const char *hostname, const char *servname);

void ra_network_close(ra_network_t network);

int ra_network_read(ra_network_t network, void *buf, size_t len);

int ra_network_write(ra_network_t network, const void *buf, size_t len);

int ra_network_writev(ra_network_t network, int n, ...);

#endif // __RA_NETWORK_H__

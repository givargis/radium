/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_network.h
 */

#ifndef _RA_NETWORK_H_
#define _RA_NETWORK_H_

#include "ra_kernel.h"

#define RA_NETWORK_WRITEV_MAX_N 4

typedef struct ra_network *ra_network_t;

typedef void (*ra_network_fnc_t)(void *ctx, ra_network_t network);

ra_network_t ra_network_listen(const char *hostname,
			       const char *servname,
			       ra_network_fnc_t fnc,
			       void *ctx);

ra_network_t ra_network_connect(const char *hostname, const char *servname);

void ra_network_close(ra_network_t network);

int ra_network_read(ra_network_t network, void *buf, size_t len);

int ra_network_write(ra_network_t network, const void *buf, size_t len);

int ra_network_writev(ra_network_t network, int n, ...);

#endif // _RA_NETWORK_H_

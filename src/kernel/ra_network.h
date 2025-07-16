/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_network.h
 */

#ifndef _RA_NETWORK_H_
#define _RA_NETWORK_H_

#include "ra_core.h"

#define RA__NETWORK_WRITEV_MAX_N 8

typedef struct ra__network *ra__network_t;

typedef void (*ra__network_fnc_t)(void *ctx, ra__network_t network);

ra__network_t ra__network_listen(const char *hostname,
				 const char *servname,
				 ra__network_fnc_t fnc,
				 void *ctx);

ra__network_t ra__network_connect(const char *hostname, const char *servname);

void ra__network_close(ra__network_t network);

int ra__network_read(ra__network_t network, void *buf, uint64_t len);

int ra__network_write(ra__network_t network, const void *buf, uint64_t len);

int ra__network_writev(ra__network_t network, int n, ...);

#endif // _RA_NETWORK_H_

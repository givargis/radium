/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_MAP_H__
#define __RA_MAP_H__

#include "ra_kernel.h"

typedef struct ra_map *ra_map_t;

typedef int (*ra_map_fnc_t)(void *ctx, const char *key, void *val);

ra_map_t ra_map_open(void);

void ra_map_close(ra_map_t map);

void ra_map_empty(ra_map_t map);

int ra_map_update(ra_map_t map, const char *key, const void *val);

void *ra_map_lookup(ra_map_t map, const char *key);

int ra_map_iterate(ra_map_t map, ra_map_fnc_t fnc, void *ctx);

uint64_t ra_map_items(ra_map_t map);

int ra_map_test(void);

#endif /* __RA_MAP_H__ */

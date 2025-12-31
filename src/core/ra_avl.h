/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_AVL_H__
#define __RA_AVL_H__

#include "ra_kernel.h"

typedef struct ra_avl *ra_avl_t;

typedef int (*ra_avl_fnc_t)(void *ctx, const char *key, void *val);

ra_avl_t ra_avl_open(void);

void ra_avl_close(ra_avl_t avl);

void ra_avl_empty(ra_avl_t avl);

int ra_avl_update(ra_avl_t avl, const char *key, const void *val);

void *ra_avl_lookup(ra_avl_t avl, const char *key);

int ra_avl_iterate(ra_avl_t avl, ra_avl_fnc_t fnc, void *ctx);

uint64_t ra_avl_items(ra_avl_t avl);

#endif /* __RA_AVL_H__ */

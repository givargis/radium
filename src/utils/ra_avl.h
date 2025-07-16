/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_avl.h
 */

#ifndef _RA_AVL_H_
#define _RA_AVL_H_

#include "../kernel/ra_kernel.h"

typedef struct ra__avl *ra__avl_t;

typedef int (*ra__avl_fnc_t)(void *ctx, const char *key, void *val);

ra__avl_t ra__avl_open(void);

void ra__avl_close(ra__avl_t avl);

void ra__avl_empty(ra__avl_t avl);

void ra__avl_remove(ra__avl_t avl, const char *key);

int ra__avl_update(ra__avl_t avl, const char *key, const void *val);

void *ra__avl_lookup(ra__avl_t avl, const char *key);

int ra__avl_iterate(ra__avl_t avl, ra__avl_fnc_t fnc, void *ctx);

uint64_t ra__avl_size(ra__avl_t avl);

int ra__avl_bist(void);

#endif // _RA_AVL_H_

/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_tree.h
 */

#ifndef _RA_INDEX_TREE_H_
#define _RA_INDEX_TREE_H_

#include "../utils/ra_utils.h"

#define RA__INDEX_TREE_MAX_KEY_LEN 32767

typedef struct ra__index_tree *ra__index_tree_t;

typedef int (*ra__index_tree_fnc_t)(void *ctx, const char *key, uint64_t ref);

int ra__index_tree_iterate(ra__index_tree_t tree,
			   ra__index_tree_fnc_t fnc,
			   void *ctx);

ra__index_tree_t ra__index_tree_open(void);

void ra__index_tree_close(ra__index_tree_t tree);

void ra__index_tree_truncate(ra__index_tree_t tree);

uint64_t *ra__index_tree_update(ra__index_tree_t tree, const char *key);

uint64_t *ra__index_tree_find(ra__index_tree_t tree, const char *key);

uint64_t *ra__index_tree_next(ra__index_tree_t tree,
			      const char *key,
			      char *okey);

uint64_t *ra__index_tree_prev(ra__index_tree_t tree,
			      const char *key,
			      char *okey);

uint64_t ra__index_tree_items(ra__index_tree_t tree);

#endif // _RA_INDEX_TREE_H_

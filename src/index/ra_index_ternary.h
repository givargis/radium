/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_ternary.h
 */

#ifndef _RA_INDEX_TERNARY_H_
#define _RA_INDEX_TERNARY_H_

#include "ra_index_tree.h"

typedef void (*ra__index_ternary_fnc_t)(void *ctx,
					char key,
					int left,
					int center,
					int right,
					uint64_t *ref);

typedef struct ra__index_ternary *ra__index_ternary_t;

ra__index_ternary_t ra__index_ternary_open(ra__index_tree_t tree);

void ra__index_ternary_close(ra__index_ternary_t ternary);

int ra__index_ternary_iterate(ra__index_ternary_t ternary,
			      ra__index_ternary_fnc_t fnc,
			      void *ctx);

uint64_t ra__index_ternary_items(ra__index_ternary_t ternary);

uint64_t ra__index_ternary_nodes(ra__index_ternary_t ternary);

#endif // _RA_INDEX_TERNARY_H_

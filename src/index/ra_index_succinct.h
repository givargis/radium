/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_index_succinct.h
 */

#ifndef _RA_INDEX_SUCCINCT_H_
#define _RA_INDEX_SUCCINCT_H_

#include "ra_index_ternary.h"

typedef struct ra__index_succinct *ra__index_succinct_t;

ra__index_succinct_t ra__index_succinct_open(ra__index_ternary_t ternary);

void ra__index_succinct_close(ra__index_succinct_t succinct);

uint64_t *ra__index_succinct_find(ra__index_succinct_t succinct,
				  const char *key);

uint64_t *ra__index_succinct_next(ra__index_succinct_t succinct,
				  const char *key,
				  char *okey);

uint64_t *ra__index_succinct_prev(ra__index_succinct_t succinct,
				  const char *key,
				  char *okey);

uint64_t ra__index_succinct_items(ra__index_succinct_t succinct);

#endif // _RA_INDEX_SUCCINCT_H_

/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_bigint.h
 */

#ifndef _RA_BIGINT_H_
#define _RA_BIGINT_H_

#include "../root/ra_root.h"

typedef struct ra_bigint *ra_bigint_t;

ra_bigint_t ra_bigint_init(const char *s);

ra_bigint_t ra_bigint_add(ra_bigint_t a, ra_bigint_t b);

ra_bigint_t ra_bigint_sub(ra_bigint_t a, ra_bigint_t b);

ra_bigint_t ra_bigint_mul(ra_bigint_t a, ra_bigint_t b);

int ra_bigint_divmod(ra_bigint_t a,
		     ra_bigint_t b,
		     ra_bigint_t *r,
		     ra_bigint_t *q);

void ra_bigint_free(ra_bigint_t a);

int ra_bigint_cmp(ra_bigint_t a, ra_bigint_t b);

int ra_bigint_is_zero(ra_bigint_t a);

int ra_bigint_is_one(ra_bigint_t a);

int ra_bigint_bist(void);

#endif // _RA_BIGINT_H_

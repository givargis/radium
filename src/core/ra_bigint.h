/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_BIGINT_H__
#define __RA_BIGINT_H__

#include <stdint.h>

typedef struct ra_bigint *ra_bigint_t;

extern const ra_bigint_t RA_BIGINT_CONST[17];

void ra_bigint_free(ra_bigint_t z);

ra_bigint_t ra_bigint_int(int64_t v);

ra_bigint_t ra_bigint_real(double r);

ra_bigint_t ra_bigint_string(const char *s);

const char *ra_bigint_print(ra_bigint_t a);

ra_bigint_t ra_bigint_add(ra_bigint_t a, ra_bigint_t b);

ra_bigint_t ra_bigint_sub(ra_bigint_t a, ra_bigint_t b);

ra_bigint_t ra_bigint_mul(ra_bigint_t a, ra_bigint_t b);

ra_bigint_t ra_bigint_div(ra_bigint_t a, ra_bigint_t b);

ra_bigint_t ra_bigint_mod(ra_bigint_t a, ra_bigint_t b);

int ra_bigint_divmod(ra_bigint_t a,
		     ra_bigint_t b,
		     ra_bigint_t *r,
		     ra_bigint_t *q);

int ra_bigint_cmp(ra_bigint_t a, ra_bigint_t b);

int ra_bigint_bits(ra_bigint_t a);

int ra_bigint_digits(ra_bigint_t a);

int ra_bigint_is_zero(ra_bigint_t a);

int ra_bigint_is_one(ra_bigint_t a);

int ra_bigint_is_square(ra_bigint_t a);

int ra_bigint_is_negative(ra_bigint_t a);

#endif /* __RA_BIGINT_H__ */

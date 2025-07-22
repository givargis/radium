//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_int256.h
//

#ifndef _RA_INT256_H_
#define _RA_INT256_H_

#include "ra_uint256.h"

#define RA_INT256_STR_LEN RA_UINT256_STR_LEN
#define RA_INT256_SET     RA_UINT256_SET
#define ra_int256         ra_uint256
#define ra_int256_init    ra_uint256_init
#define ra_int256_is_zero ra_uint256_is_zero
#define ra_int256_shl_    ra_uint256_shl_
#define ra_int256_shl     ra_uint256_shl
#define ra_int256_set_bit ra_uint256_set_bit
#define ra_int256_clr_bit ra_uint256_clr_bit
#define ra_int256_get_bit ra_uint256_get_bit
#define ra_int256_not     ra_uint256_not
#define ra_int256_and     ra_uint256_and
#define ra_int256_xor     ra_uint256_xor
#define ra_int256_or      ra_uint256_or

void ra_int256_string(const struct ra_int256 *a, char *s);

int ra_int256_cmp(const struct ra_int256 *a_, const struct ra_int256 *b);

int ra_int256_neg(struct ra_int256 *z, const struct ra_int256 *a);

int ra_int256_add(struct ra_int256 *z,
		  const struct ra_int256 *a,
		  const struct ra_int256 *b);

int ra_int256_sub(struct ra_int256 *z,
		  const struct ra_int256 *a,
		  const struct ra_int256 *b);

int ra_int256_mul(struct ra_int256 *z,
		  const struct ra_int256 *a,
		  const struct ra_int256 *b);

int ra_int256_div(struct ra_int256 *z,
		  const struct ra_int256 *a,
		  const struct ra_int256 *b);

int ra_int256_mod(struct ra_int256 *z,
		  const struct ra_int256 *a,
		  const struct ra_int256 *b);

void ra_int256_shr_(struct ra_int256 *z, const struct ra_int256 *a, int n);

static inline int
ra_int256_is_neg(const struct ra_int256 *a)
{
	assert( a );

	return (a->hh & 0x8000000000000000) ? 1 : 0;
}

static inline void
ra_int256_shr(struct ra_int256 *z,
	      const struct ra_int256 *a,
	      const struct ra_int256 *b)
{
	assert( z && a && b );

	ra_int256_shr_(z, a, b->ll % 256);
}

#endif // _RA_INT256_H_

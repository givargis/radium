/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_int256.h
 */

#ifndef _RA_INT256_H_
#define _RA_INT256_H_

#include "ra_uint256.h"

#define RA__INT256_STR_LEN RA__UINT256_STR_LEN
#define RA__INT256_SET     RA__UINT256_SET
#define ra__int256         ra__uint256
#define ra__int256_init    ra__uint256_init
#define ra__int256_is_zero ra__uint256_is_zero
#define ra__int256_shl_    ra__uint256_shl_
#define ra__int256_shl     ra__uint256_shl
#define ra__int256_set_bit ra__uint256_set_bit
#define ra__int256_clr_bit ra__uint256_clr_bit
#define ra__int256_get_bit ra__uint256_get_bit
#define ra__int256_not     ra__uint256_not
#define ra__int256_and     ra__uint256_and
#define ra__int256_xor     ra__uint256_xor
#define ra__int256_or      ra__uint256_or

void ra__int256_string(const struct ra__int256 *a, char *s);

int ra__int256_cmp(const struct ra__int256 *a_, const struct ra__int256 *b);

int ra__int256_neg(struct ra__int256 *z, const struct ra__int256 *a);

int ra__int256_add(struct ra__int256 *z,
		   const struct ra__int256 *a,
		   const struct ra__int256 *b);

int ra__int256_sub(struct ra__int256 *z,
		   const struct ra__int256 *a,
		   const struct ra__int256 *b);

int ra__int256_mul(struct ra__int256 *z,
		   const struct ra__int256 *a,
		   const struct ra__int256 *b);

int ra__int256_div(struct ra__int256 *z,
		   const struct ra__int256 *a,
		   const struct ra__int256 *b);

int ra__int256_mod(struct ra__int256 *z,
		   const struct ra__int256 *a,
		   const struct ra__int256 *b);

void ra__int256_shr_(struct ra__int256 *z, const struct ra__int256 *a, int n);

int ra__int256_bist(void);

static inline int
ra__int256_is_neg(const struct ra__int256 *a)
{
	assert( a );

	return (a->hh & 0x8000000000000000) ? 1 : 0;
}

static inline void
ra__int256_shr(struct ra__int256 *z,
	       const struct ra__int256 *a,
	       const struct ra__int256 *b)
{
	assert( z && a && b );

	ra__int256_shr_(z, a, b->ll % 256);
}

#endif // _RA_INT256_H_

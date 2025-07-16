/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_uint256.h
 */

#ifndef _RA_UINT256_H_
#define _RA_UINT256_H_

#include "../kernel/ra_kernel.h"

#define RA__UINT256_STR_LEN 81

#define RA__UINT256_SET(z,v)				\
	do {						\
		memset(z,				\
		       0,				\
		       sizeof (struct ra__uint256));	\
		(z)->ll = (uint64_t)(v);		\
	}						\
	while (0)

struct ra__uint256 {
	uint64_t hh;
	uint64_t hl;
	uint64_t lh;
	uint64_t ll;
};

int ra__uint256_init(struct ra__uint256 *z, const char *s);

void ra__uint256_string(const struct ra__uint256 *a, char *s);

void ra__uint256_add_(struct ra__uint256 *z,
		      const struct ra__uint256 *a,
		      const struct ra__uint256 *b);

int ra__uint256_add(struct ra__uint256 *z,
		    const struct ra__uint256 *a,
		    const struct ra__uint256 *b);

int ra__uint256_sub(struct ra__uint256 *z,
		    const struct ra__uint256 *a,
		    const struct ra__uint256 *b);

void ra__uint256_mulf(struct ra__uint256 *zh,
		      struct ra__uint256 *zl,
		      const struct ra__uint256 *a,
		      const struct ra__uint256 *b);

int ra__uint256_mul(struct ra__uint256 *z,
		    const struct ra__uint256 *a,
		    const struct ra__uint256 *b);

int ra__uint256_divmod(struct ra__uint256 *zq,
		       struct ra__uint256 *zr,
		       const struct ra__uint256 *a,
		       const struct ra__uint256 *b);

void ra__uint256_shl_(struct ra__uint256 *z,
		      const struct ra__uint256 *a,
		      int n);

void ra__uint256_shr_(struct ra__uint256 *z,
		      const struct ra__uint256 *a,
		      int n);

int ra__uint256_bist(void);

static inline int
ra__uint256_is_zero(const struct ra__uint256 *a)
{
	assert( a );

	return ra__is_zero(a, sizeof (struct ra__uint256));
}

static inline int
ra__uint256_cmp(const struct ra__uint256 *a, const struct ra__uint256 *b)
{
	assert( a && b );

	if (a->hh > b->hh) return +1;
	if (a->hh < b->hh) return -1;
	if (a->hl > b->hl) return +1;
	if (a->hl < b->hl) return -1;
	if (a->lh > b->lh) return +1;
	if (a->lh < b->lh) return -1;
	if (a->ll > b->ll) return +1;
	if (a->ll < b->ll) return -1;
	return 0;
}

static inline int
ra__uint256_div(struct ra__uint256 *z,
		const struct ra__uint256 *a,
		const struct ra__uint256 *b)
{
	struct ra__uint256 r;

	assert( z && a && b );

	return ra__uint256_divmod(z, &r, a, b);
}

static inline int
ra__uint256_mod(struct ra__uint256 *z,
		const struct ra__uint256 *a,
		const struct ra__uint256 *b)
{
	struct ra__uint256 q;

	assert( z && a && b );

	return ra__uint256_divmod(&q, z, a, b);
}

static inline void
ra__uint256_shl(struct ra__uint256 *z,
		const struct ra__uint256 *a,
		const struct ra__uint256 *b)
{
	assert( z && a && b );

	ra__uint256_shl_(z, a, b->ll & 0xff);
}

static inline void
ra__uint256_shr(struct ra__uint256 *z,
		const struct ra__uint256 *a,
		const struct ra__uint256 *b)
{
	assert( z && a && b );

	ra__uint256_shr_(z, a, b->ll & 0xff);
}

static inline void
ra__uint256_set_bit(struct ra__uint256 *a, int i)
{
	assert( a );
	assert( (0 <= i) && (255 >= i) );

	if      ( 64 > i) a->ll |= (1LU <<  i       );
	else if (128 > i) a->lh |= (1LU << (i -  64));
	else if (192 > i) a->hl |= (1LU << (i - 128));
	else /*--------*/ a->hh |= (1LU << (i - 192));
}

static inline void
ra__uint256_clr_bit(struct ra__uint256 *a, int i)
{
	assert( a );
	assert( (0 <= i) && (255 >= i) );

	if      ( 64 > i) a->ll &= ~(1LU <<  i       );
	else if (128 > i) a->lh &= ~(1LU << (i -  64));
	else if (192 > i) a->hl &= ~(1LU << (i - 128));
	else /*--------*/ a->hh &= ~(1LU << (i - 192));
}

static inline int
ra__uint256_get_bit(const struct ra__uint256 *a, int i)
{
	assert( a );
	assert( (0 <= i) && (255 >= i) );

	if      ( 64 > i) return (a->ll & (1LU <<  i       )) ? 1 : 0;
	else if (128 > i) return (a->lh & (1LU << (i -  64))) ? 1 : 0;
	else if (192 > i) return (a->hl & (1LU << (i - 128))) ? 1 : 0;
	else /*--------*/ return (a->hh & (1LU << (i - 192))) ? 1 : 0;
}

static inline void
ra__uint256_not(struct ra__uint256 *z, const struct ra__uint256 *a)
{
	assert( z && a );

	z->hh = ~a->hh;
	z->hl = ~a->hl;
	z->lh = ~a->lh;
	z->ll = ~a->ll;
}

static inline void
ra__uint256_and(struct ra__uint256 *z,
		const struct ra__uint256 *a,
		const struct ra__uint256 *b)
{
	assert( z && a && b );

	z->hh = a->hh & b->hh;
	z->hl = a->hl & b->hl;
	z->lh = a->lh & b->lh;
	z->ll = a->ll & b->ll;
}

static inline void
ra__uint256_xor(struct ra__uint256 *z,
		const struct ra__uint256 *a,
		const struct ra__uint256 *b)
{
	assert( z && a && b );

	z->hh = a->hh ^ b->hh;
	z->hl = a->hl ^ b->hl;
	z->lh = a->lh ^ b->lh;
	z->ll = a->ll ^ b->ll;
}

static inline void
ra__uint256_or(struct ra__uint256 *z,
	       const struct ra__uint256 *a,
	       const struct ra__uint256 *b)
{
	assert( z && a && b );

	z->hh = a->hh | b->hh;
	z->hl = a->hl | b->hl;
	z->lh = a->lh | b->lh;
	z->ll = a->ll | b->ll;
}

#endif // _RA_UINT256_H_

//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_int256.c
//

#include "ra_kernel.h"
#include "ra_int256.h"

void
ra_int256_string(const struct ra_int256 *a, char *s)
{
	static const struct ra_uint256 ONE = { 0, 0, 0, 1 };
	struct ra_int256 z;

	assert( a && s );

	if (ra_int256_is_neg(a)) {
		(*s++) = '-';
		ra_uint256_not(&z, a);
		ra_uint256_add_(&z, &z, &ONE);
		ra_uint256_string(&z, s);
	}
	else {
		ra_uint256_string(a, s);
	}
}

int
ra_int256_cmp(const struct ra_int256 *a_, const struct ra_int256 *b_)
{
	struct ra_uint256 a, b, o;
	int sa, sb;

	assert( a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra_int256_is_neg(a_)) {
		sa = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&a, a_);
		ra_uint256_add_(&a, &a, &o);
	}
	if (ra_int256_is_neg(b_)) {
		sb = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&b, b_);
		ra_uint256_add_(&b, &b, &o);
	}
	if (sa && sb) {
		return ra_uint256_cmp(&a, &b) * -1;
	}
	if (!sa && !sb) {
		return ra_uint256_cmp(&a, &b);
	}
	return sa ? -1 : +1;
}

int
ra_int256_neg(struct ra_int256 *z, const struct ra_int256 *a)
{
	static const struct ra_uint256 ONE = { 0, 0, 0, 1 };
	int sa;

	assert( z && a );

	sa = ra_int256_is_neg(a);
	ra_uint256_not(z, a);
	ra_uint256_add_(z, z, &ONE);
	if (sa && ra_int256_is_neg(z)) {
		RA_TRACE("arithmetic");
		return -1;
	}
	return 0;
}

int
ra_int256_add(struct ra_int256 *z,
	      const struct ra_int256 *a,
	      const struct ra_int256 *b)
{
	int sa, sb;

	assert( z && a && b );

	sa = ra_int256_is_neg(a);
	sb = ra_int256_is_neg(b);
	ra_uint256_add_(z, a, b);
	if ((sa == sb) && (sa != ra_int256_is_neg(z))) {
		RA_TRACE("arithmetic");
		return -1;
	}
	return 0;
}

int
ra_int256_sub(struct ra_int256 *z,
	      const struct ra_int256 *a,
	      const struct ra_int256 *b)
{
	struct ra_uint256 t;

	assert( z && a && b );

	return ra_int256_neg(&t, b) || ra_int256_add(z, a, &t);
}

int
ra_int256_mul(struct ra_int256 *z,
	      const struct ra_int256 *a_,
	      const struct ra_int256 *b_)
{
	struct ra_uint256 a, b, o;
	int sa, sb;

	assert( z && a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra_int256_is_neg(a_)) {
		sa = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&a, a_);
		ra_uint256_add_(&a, &a, &o);
	}
	if (ra_int256_is_neg(b_)) {
		sb = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&b, b_);
		ra_uint256_add_(&b, &b, &o);
	}
	if (ra_uint256_mul(z, &a, &b)) {
		RA_TRACE(NULL);
		return -1;
	}
	if (sa ^ sb) {
		ra_uint256_not(z, z);
		ra_uint256_add_(z, z, &o);
	}
	return 0;
}

int
ra_int256_div(struct ra_int256 *z,
	      const struct ra_int256 *a_,
	      const struct ra_int256 *b_)
{
	struct ra_uint256 a, b, o;
	int sa, sb;

	assert( z && a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra_int256_is_neg(a_)) {
		sa = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&a, a_);
		ra_uint256_add_(&a, &a, &o);
	}
	if (ra_int256_is_neg(b_)) {
		sb = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&b, b_);
		ra_uint256_add_(&b, &b, &o);
	}
	if (ra_uint256_div(z, &a, &b)) {
		RA_TRACE(NULL);
		return -1;
	}
	if (sa ^ sb) {
		ra_uint256_not(z, z);
		ra_uint256_add_(z, z, &o);
	}
	return 0;
}

int
ra_int256_mod(struct ra_int256 *z,
	      const struct ra_int256 *a_,
	      const struct ra_int256 *b_)
{
	struct ra_uint256 a, b, o;
	int sa, sb;

	assert( z && a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra_int256_is_neg(a_)) {
		sa = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&a, a_);
		ra_uint256_add_(&a, &a, &o);
	}
	if (ra_int256_is_neg(b_)) {
		sb = 1;
		RA_UINT256_SET(&o, 1);
		ra_uint256_not(&b, b_);
		ra_uint256_add_(&b, &b, &o);
	}
	if (ra_uint256_mod(z, &a, &b)) {
		RA_TRACE(NULL);
		return -1;
	}
	if (sa) {
		ra_uint256_not(z, z);
		ra_uint256_add_(z, z, &o);
	}
	return 0;
}

void
ra_int256_shr_(struct ra_int256 *z, const struct ra_int256 *a, int n)
{
	int sa;

	assert( z && a );
	assert( (0 <= n) && (255 >= n) );

	sa = ra_int256_is_neg(a);
	ra_uint256_shr_(z, a, n);
	if (sa) {
		for (int i=0; i<n; ++i) {
			ra_int256_set_bit(z, 255 - i);
		}
	}
}

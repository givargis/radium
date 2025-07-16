/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_int256.c
 */

#include "ra_int256.h"

void
ra__int256_string(const struct ra__int256 *a, char *s)
{
	static const struct ra__uint256 ONE = { 0, 0, 0, 1 };
	struct ra__int256 z;

	assert( a && s );

	if (ra__int256_is_neg(a)) {
		(*s++) = '-';
		ra__uint256_not(&z, a);
		ra__uint256_add_(&z, &z, &ONE);
		ra__uint256_string(&z, s);
	}
	else {
		ra__uint256_string(a, s);
	}
}

int
ra__int256_cmp(const struct ra__int256 *a_, const struct ra__int256 *b_)
{
	struct ra__uint256 a, b, o;
	int sa, sb;

	assert( a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra__int256_is_neg(a_)) {
		sa = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&a, a_);
		ra__uint256_add_(&a, &a, &o);
	}
	if (ra__int256_is_neg(b_)) {
		sb = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&b, b_);
		ra__uint256_add_(&b, &b, &o);
	}
	if (sa && sb) {
		return ra__uint256_cmp(&a, &b) * -1;
	}
	if (!sa && !sb) {
		return ra__uint256_cmp(&a, &b);
	}
	return sa ? -1 : +1;
}

int
ra__int256_neg(struct ra__int256 *z, const struct ra__int256 *a)
{
	static const struct ra__uint256 ONE = { 0, 0, 0, 1 };
	int sa;

	assert( z && a );

	sa = ra__int256_is_neg(a);
	ra__uint256_not(z, a);
	ra__uint256_add_(z, z, &ONE);
	if (sa && ra__int256_is_neg(z)) {
		RA__ERROR_TRACE(RA__ERROR_ARITHMETIC);
		return -1;
	}
	return 0;
}

int
ra__int256_add(struct ra__int256 *z,
	       const struct ra__int256 *a,
	       const struct ra__int256 *b)
{
	int sa, sb;

	assert( z && a && b );

	sa = ra__int256_is_neg(a);
	sb = ra__int256_is_neg(b);
	ra__uint256_add_(z, a, b);
	if ((sa == sb) && (sa != ra__int256_is_neg(z))) {
		RA__ERROR_TRACE(RA__ERROR_ARITHMETIC);
		return -1;
	}
	return 0;
}

int
ra__int256_sub(struct ra__int256 *z,
	       const struct ra__int256 *a,
	       const struct ra__int256 *b)
{
	struct ra__uint256 t;

	assert( z && a && b );

	return ra__int256_neg(&t, b) || ra__int256_add(z, a, &t);
}

int
ra__int256_mul(struct ra__int256 *z,
	       const struct ra__int256 *a_,
	       const struct ra__int256 *b_)
{
	struct ra__uint256 a, b, o;
	int sa, sb;

	assert( z && a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra__int256_is_neg(a_)) {
		sa = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&a, a_);
		ra__uint256_add_(&a, &a, &o);
	}
	if (ra__int256_is_neg(b_)) {
		sb = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&b, b_);
		ra__uint256_add_(&b, &b, &o);
	}
	if (ra__uint256_mul(z, &a, &b)) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	if (sa ^ sb) {
		ra__uint256_not(z, z);
		ra__uint256_add_(z, z, &o);
	}
	return 0;
}

int
ra__int256_div(struct ra__int256 *z,
	       const struct ra__int256 *a_,
	       const struct ra__int256 *b_)
{
	struct ra__uint256 a, b, o;
	int sa, sb;

	assert( z && a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra__int256_is_neg(a_)) {
		sa = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&a, a_);
		ra__uint256_add_(&a, &a, &o);
	}
	if (ra__int256_is_neg(b_)) {
		sb = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&b, b_);
		ra__uint256_add_(&b, &b, &o);
	}
	if (ra__uint256_div(z, &a, &b)) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	if (sa ^ sb) {
		ra__uint256_not(z, z);
		ra__uint256_add_(z, z, &o);
	}
	return 0;
}

int
ra__int256_mod(struct ra__int256 *z,
	       const struct ra__int256 *a_,
	       const struct ra__int256 *b_)
{
	struct ra__uint256 a, b, o;
	int sa, sb;

	assert( z && a_ && b_ );

	a = (*a_);
	b = (*b_);
	sa = sb = 0;
	if (ra__int256_is_neg(a_)) {
		sa = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&a, a_);
		ra__uint256_add_(&a, &a, &o);
	}
	if (ra__int256_is_neg(b_)) {
		sb = 1;
		RA__UINT256_SET(&o, 1);
		ra__uint256_not(&b, b_);
		ra__uint256_add_(&b, &b, &o);
	}
	if (ra__uint256_mod(z, &a, &b)) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	if (sa) {
		ra__uint256_not(z, z);
		ra__uint256_add_(z, z, &o);
	}
	return 0;
}

void
ra__int256_shr_(struct ra__int256 *z, const struct ra__int256 *a, int n)
{
	int sa;

	assert( z && a );
	assert( (0 <= n) && (255 >= n) );

	sa = ra__int256_is_neg(a);
	ra__uint256_shr_(z, a, n);
	if (sa) {
		for (int i=0; i<n; ++i) {
			ra__int256_set_bit(z, 255 - i);
		}
	}
}

int
ra__int256_bist(void)
{
	const int N = 123456;
	struct ra__uint256 a;
	struct ra__uint256 b;
	struct ra__uint256 q;
	struct ra__uint256 r;
	char buf[96];

	// basic

	if (ra__int256_init(&a, 0) ||
	    ra__int256_is_neg(&a) ||
	    ra__int256_neg(&b, &a) ||
	    ra__int256_is_neg(&b)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__int256_string(&a, buf);
	if (strcmp("0", buf)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__int256_string(&b, buf);
	if (strcmp("0", buf)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// basic

	if (ra__int256_init(&a,
			    "0x"
			    "ffffffffffffffff"
			    "ffffffffffffffff"
			    "ffffffffffffffff"
			    "ffffffffffffffff") ||
	    !ra__int256_is_neg(&a) ||
	    ra__int256_neg(&b, &a) ||
	    ra__int256_is_neg(&b)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__int256_string(&a, buf);
	if (strcmp("-1", buf)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__int256_string(&b, buf);
	if (strcmp("1", buf)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// basic

	if (ra__int256_init(&a, "1") ||
	    ra__int256_is_neg(&a) ||
	    ra__int256_neg(&b, &a) ||
	    !ra__int256_is_neg(&b)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__int256_string(&a, buf);
	if (strcmp("1", buf)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__int256_string(&b, buf);
	if (strcmp("-1", buf)) {
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// a == (a / b) * b + (a % b)

	for (int i=0; i<N; ++i) {
		a.hh = ((uint64_t)rand() << 32) + rand();
		a.hl = ((uint64_t)rand() << 32) + rand();
		a.lh = ((uint64_t)rand() << 32) + rand();
		a.ll = ((uint64_t)rand() << 32) + rand();
		b.hh = ((uint64_t)rand() << 32) + rand();
		b.hl = ((uint64_t)rand() << 32) + rand();
		b.lh = ((uint64_t)rand() << 32) + rand();
		b.ll = ((uint64_t)rand() << 32) + rand();
		b.ll = b.ll ? b.ll : 1;
		if (ra__int256_div(&q, &a, &b) ||
		    ra__int256_mod(&r, &a, &b) ||
		    ra__int256_mul(&q, &q, &b) ||
		    ra__int256_add(&b, &q, &r) ||
		    ra__int256_cmp(&a, &b)) {
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
	}
	return 0;
}

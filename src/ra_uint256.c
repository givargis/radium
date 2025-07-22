//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_uint256.c
//

#include <stdlib.h>
#include <ctype.h>

#include "ra_kernel.h"
#include "ra_uint256.h"

static int
hex2int(int c)
{
	c = tolower(c);
	if (('0' <= c) && ('9' >= c)) {
		return c - '0';
	}
	if (('a' <= c) && ('f' >= c)) {
		return 15 + c - 'f';
	}
	return -1;
}

static int
oct2int(int c)
{
	c = tolower(c);
	if (('0' <= c) && ('7' >= c)) {
		return c - '0';
	}
	return -1;
}

static int
dec2int(int c)
{
	if (('0' <= c) && ('9' >= c)) {
		return c - '0';
	}
	return -1;
}

static int
add64(uint64_t *za, uint64_t b)
{
	if ((0xffffffffffffffff - (*za)) < b) {
		(*za) += b;
		return 1;
	}
	(*za) += b;
	return 0;
}

static int
sub64(uint64_t *za, uint64_t b)
{
	if ((*za) < b) {
		(*za) -= b;
		return 1;
	}
	(*za) -= b;
	return 0;
}

static int
add128(uint64_t *zah, uint64_t *zal, uint64_t bh, uint64_t bl)
{
	if (add64(zal, bl)) {
		return add64(zah, 1) + add64(zah, bh);
	}
	return add64(zah, bh);
}

static int
sub128(uint64_t *zah, uint64_t *zal, uint64_t bh, uint64_t bl)
{
	if (sub64(zal, bl)) {
		return sub64(zah, 1) + sub64(zah, bh);
	}
	return sub64(zah, bh);
}

static int
add256(struct ra_uint256 *za, const struct ra_uint256 *b)
{
	assert( za && b );

	if (add128(&za->lh, &za->ll, b->lh, b->ll)) {
		return add128(&za->hh, &za->hl, 0, 1) +
			add128(&za->hh, &za->hl, b->hh, b->hl);
	}
	return add128(&za->hh, &za->hl, b->hh, b->hl);
}

static int
sub256(struct ra_uint256 *za, const struct ra_uint256 *b)
{
	assert( za && b );

	if (sub128(&za->lh, &za->ll, b->lh, b->ll)) {
		return sub128(&za->hh, &za->hl, 0, 1) +
			sub128(&za->hh, &za->hl, b->hh, b->hl);
	}
	return sub128(&za->hh, &za->hl, b->hh, b->hl);
}

static void
add128_no_carry(uint64_t *zah, uint64_t *zal, uint64_t b)
{
	if (add64(zal, b)) {
		if (add64(zah, 1)) {
			RA_TRACE("software (halting)");
			exit(-1);
		}
	}
}

static void
add256_no_carry(struct ra_uint256 *za, uint64_t bh, uint64_t bl)
{
	if (add128(&za->lh, &za->ll, bh, bl)) {
		if (add128(&za->hh, &za->hl, 0, 1)) {
			RA_TRACE("software (halting)");
			exit(-1);
		}
	}
}

static void
mul64(uint64_t *zh, uint64_t *zl, uint64_t a, uint64_t b)
{
#ifdef __SIZEOF_INT128__
	__uint128_t t;

	t = (__uint128_t)a * (__uint128_t)b;
	(*zh) = (uint64_t)(t >> 64);
	(*zl) = (uint64_t)(t      );
#else
	uint64_t ah, al, bh, bl, t1, t2, t3, t4;

	ah = a >> 32;
	al = a & 0xffffffff;
	bh = b >> 32;
	bl = b & 0xffffffff;
	t1 = bl * al;
	t2 = bl * ah;
	t3 = bh * al;
	t4 = bh * ah;
	t2 += t1 >> 32;
	if (add64(&t3, t2)) {
		t4 += 1LU << 32;
	}
	t4 += t3 >> 32;
	(*zh) = t4;
	(*zl) = (t3 << 32) | (t1 & 0xffffffff);
#endif
}

static void
mul128(struct ra_uint256 *z,
       uint64_t ah,
       uint64_t al,
       uint64_t bh,
       uint64_t bl)
{
	uint64_t t1h, t1l, t2h, t2l, t3h, t3l, t4h, t4l;

	mul64(&t1h, &t1l, bl, al);
	mul64(&t2h, &t2l, bl, ah);
	mul64(&t3h, &t3l, bh, al);
	mul64(&t4h, &t4l, bh, ah);
	add128_no_carry(&t2h, &t2l, t1h);
	if (add128(&t3h, &t3l, t2h, t2l)) {
		if (add64(&t4h, 1)) {
			RA_TRACE("software");
			exit(-1);
		}
	}
	add128_no_carry(&t4h, &t4l, t3h);
	z->hh = t4h;
	z->hl = t4l;
	z->lh = t3l;
	z->ll = t1l;
}

static void
shl128(uint64_t *zh, uint64_t *zl, uint64_t ah, uint64_t al, int n)
{
	int m;

	while (n) {
		m  = (63 < n) ? 63 : n;
		n -= m;
		ah = ah << m;
		ah = ah | (al >> (64 - m));
		al <<= m;
	}
	(*zh) = ah;
	(*zl) = al;
}

static void
shr128(uint64_t *zh, uint64_t *zl, uint64_t ah, uint64_t al, int n)
{
	int m;

	while (n) {
		m  = (63 < n) ? 63 : n;
		n -= m;
		al = al >> m;
		al = al | (ah << (64 - m));
		ah >>= m;
	}
	(*zh) = ah;
	(*zl) = al;
}

int
ra_uint256_init(struct ra_uint256 *z, const char *s)
{
	struct ra_uint256 m, a;
	int (*p2v)(int);
	int v;

	assert( z );

	RA_UINT256_SET(z, 0);
	if (s && strlen(s)) {
		if (('0' == s[0]) && (('x' == s[1]) || ('X' == s[1]))) {
			if (!s[2]) {
				RA_TRACE("argument");
				return -1;
			}
			s += 2;
			p2v = hex2int;
			RA_UINT256_SET(&m, 16);
		}
		else if (('0' == s[0]) && ('\0' != s[1])) {
			s += 1;
			p2v = oct2int;
			RA_UINT256_SET(&m, 8);
		}
		else {
			p2v = dec2int;
			RA_UINT256_SET(&m, 10);
		}
		while (*s) {
			if (0 > (v = p2v((unsigned char)(*s++)))) {
				RA_TRACE("argument");
				return -1;
			}
			RA_UINT256_SET(&a, v);
			if (ra_uint256_mul(z, z, &m) ||
			    ra_uint256_add(z, z, &a)) {
				RA_TRACE(NULL);
				return -1;
			}
		}
	}
	return 0;
}

void
ra_uint256_string(const struct ra_uint256 *a, char *s)
{
	struct ra_uint256 q, r, m;
	uint64_t stack[10];
	int i, n;

	assert( a && s );

	i = 0;
	q = (*a);
	RA_UINT256_SET(&m, 1000000000);
	while (!ra_uint256_is_zero(&q)) {
		if (ra_uint256_divmod(&q, &r, &q, &m)) {
			RA_TRACE("software (halting)");
			exit(-1);
		}
		stack[i++] = r.ll;
		assert( i < (int)RA_ARRAY_SIZE(stack) );
	}
	ra_sprintf(s, RA_UINT256_STR_LEN, "0");
	if (i) {
		ra_sprintf(s,
			   RA_UINT256_STR_LEN,
			   "%lu",
			   (unsigned long)stack[--i]);
		while (i) {
			n = (int)strlen(s);
			ra_sprintf(s + n,
				   RA_UINT256_STR_LEN - n,
				   "%09lu",
				   (unsigned long)stack[--i]);
		}
	}
}

void
ra_uint256_add_(struct ra_uint256 *z,
		const struct ra_uint256 *a,
		const struct ra_uint256 *b)
{
	assert( z && a && b );

	(*z) = (*a);
	add256(z, b);
}

int
ra_uint256_add(struct ra_uint256 *z,
	       const struct ra_uint256 *a,
	       const struct ra_uint256 *b)
{
	assert( z && a && b );

	(*z) = (*a);
	if (add256(z, b)) {
		RA_TRACE("arithmetic");
		return -1;
	}
	return 0;
}

int
ra_uint256_sub(struct ra_uint256 *z,
	       const struct ra_uint256 *a,
	       const struct ra_uint256 *b)
{
	assert( z && a && b );

	(*z) = (*a);
	if (sub256(z, b)) {
		RA_TRACE("arithmetic");
		return -1;
	}
	return 0;
}

void
ra_uint256_mulf(struct ra_uint256 *zh,
		struct ra_uint256 *zl,
		const struct ra_uint256 *a,
		const struct ra_uint256 *b)
{
	struct ra_uint256 t1, t2, t3, t4;

	assert( zh && zl && a && b );

	mul128(&t1, b->lh, b->ll, a->lh, a->ll);
	mul128(&t2, b->lh, b->ll, a->hh, a->hl);
	mul128(&t3, b->hh, b->hl, a->lh, a->ll);
	mul128(&t4, b->hh, b->hl, a->hh, a->hl);
	add256_no_carry(&t2, t1.hh, t1.hl);
	if (add256(&t3, &t2)) {
		if (add128(&t4.hh, &t4.hl, 0, 1)) {
			RA_TRACE("software (halting)");
			exit(-1);
		}
	}
	add256_no_carry(&t4, t3.hh, t3.hl);
	zh->hh = t4.hh;
	zh->hl = t4.hl;
	zh->lh = t4.lh;
	zh->ll = t4.ll;
	zl->hh = t3.lh;
	zl->hl = t3.ll;
	zl->lh = t1.lh;
	zl->ll = t1.ll;
}

int
ra_uint256_mul(struct ra_uint256 *z,
	       const struct ra_uint256 *a,
	       const struct ra_uint256 *b)
{
	struct ra_uint256 t1, t2, t3;

	assert( z && a && b );

	mul128(&t1, b->lh, b->ll, a->lh, a->ll);
	mul128(&t2, b->lh, b->ll, a->hh, a->hl);
	mul128(&t3, b->hh, b->hl, a->lh, a->ll);
	add256_no_carry(&t2, t1.hh, t1.hl);
	if (add256(&t3, &t2)) {
		RA_TRACE("arithmetic");
		return -1;
	}
	z->hh = t3.lh;
	z->hl = t3.ll;
	z->lh = t1.lh;
	z->ll = t1.ll;
	return 0;
}

int
ra_uint256_divmod(struct ra_uint256 *zq,
		  struct ra_uint256 *zr,
		  const struct ra_uint256 *a_,
		  const struct ra_uint256 *b_)
{
	struct ra_uint256 a, b;

	assert( zq && zr && a_ && b_ );

	a = (*a_);
	b = (*b_);
	RA_UINT256_SET(zq, 0);
	RA_UINT256_SET(zr, 0);
	if (ra_uint256_is_zero(b_)) {
		RA_TRACE("arithmetic");
		return -1;
	}
	for (int i=255; i>=0; --i) {
		ra_uint256_shl_(zr, zr, 1);
		if (ra_uint256_get_bit(&a, i)) {
			ra_uint256_set_bit(zr, 0);
		}
		if (0 <= ra_uint256_cmp(zr, &b)) {
			if (ra_uint256_sub(zr, zr, &b)) {
				RA_TRACE("software");
				return -1;
			}
			ra_uint256_set_bit(zq, i);
		}
	}
	return 0;
}

void
ra_uint256_shl_(struct ra_uint256 *z, const struct ra_uint256 *a, int n)
{
	uint64_t h, l;
	int m;

	assert( z && a );
	assert( (0 <= n) && (255 >= n) );

	(*z) = (*a);
	while (n) {
		m  = (127 < n) ? 127 : n;
		n -= m;
		shl128(&z->hh, &z->hl, z->hh, z->hl, m);
		shr128(&h, &l, z->lh, z->ll, 128 - m);
		z->hh |= h;
		z->hl |= l;
		shl128(&z->lh, &z->ll, z->lh, z->ll, m);
	}
}

void
ra_uint256_shr_(struct ra_uint256 *z, const struct ra_uint256 *a, int n)
{
	uint64_t h, l;
	int m;

	assert( z && a );
	assert( (0 <= n) && (255 >= n) );

	(*z) = (*a);
	while (n) {
		m  = (127 < n) ? 127 : n;
		n -= m;
		shr128(&z->lh, &z->ll, z->lh, z->ll, m);
		shl128(&h, &l, z->hh, z->hl, 128 - m);
		z->lh |= h;
		z->ll |= l;
		shr128(&z->hh, &z->hl, z->hh, z->hl, m);
	}
}

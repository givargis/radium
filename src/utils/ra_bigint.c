/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_bigint.c
 */

#include "ra_bigint.h"

#define MAX_DIGITS 8192

#define SET(z, v)						\
	do {							\
		memset((z)->digits,				\
		       0,					\
		       (z)->width * sizeof ((z)->digits[0]));	\
		(z)->digits[0] = (uint64_t)(v);			\
	} while (0)

#define NORMALIZE(z)							\
	do {								\
		while ((z)->width && !(z)->digits[(z)->width - 1]) {	\
			--(z)->width;					\
		}							\
		if (!z->width) {					\
			(z)->sign = 0;					\
			free((z)->digits);				\
			(z)->digits = NULL;				\
		}							\
	} while (0)

#define IS_ONE(a) ( (1 == (a)->width) && (1 == (a)->digits[0]) )

struct ra_bigint {
	int sign;
	int width;
	uint64_t *digits;
};

static uint64_t _C[]  = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

static struct ra_bigint C[] = {
	{ 0,0, NULL    }, { 0,1, &_C[ 1] }, { 0,1, &_C[ 2] }, { 0,1, &_C[ 3] },
	{ 0,1, &_C[ 4] }, { 0,1, &_C[ 5] }, { 0,1, &_C[ 6] }, { 0,1, &_C[ 7] },
	{ 0,1, &_C[ 8] }, { 0,1, &_C[ 9] }, { 0,1, &_C[10] }, { 0,1, &_C[11] },
	{ 0,1, &_C[12] }, { 0,1, &_C[13] }, { 0,1, &_C[14] }, { 0,1, &_C[15] },
	{ 0,1, &_C[16] }
};

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
bin2int(int c)
{
	if (('0' <= c) && ('1' >= c)) {
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

static void
mul(uint64_t *h, uint64_t *l, uint64_t a, uint64_t b)
{
	// hardware

#ifdef __SIZEOF_INT128__
	__uint128_t t = (__uint128_t)a * (__uint128_t)b;
	(*h) = (uint64_t)(t >> 64);
	(*l) = (uint64_t)t;
	return;
#endif

	// emulated

	uint64_t ah = a >> 32;
	uint64_t al = a & 0xffffffff;
	uint64_t bh = b >> 32;
	uint64_t bl = b & 0xffffffff;
	uint64_t t1 = bl * al;
	uint64_t t2 = bl * ah;
	uint64_t t3 = bh * al;
	uint64_t t4 = bh * ah;
	t2 += t1 >> 32;
	t3 += t2;
	if (t3 < t2) {
		t4 += 1LU << 32;
	}
	t4 += t3 >> 32;
	(*h) = t4;
	(*l) = (t3 << 32) | (t1 & 0xffffffff);
}

static struct ra_bigint *
allocate(int width)
{
	struct ra_bigint *z;

	assert( 0 <= width );

	if (MAX_DIGITS < width) {
		RA_TRACE("integer too large");
		return NULL;
	}
	if (!(z = malloc(sizeof (struct ra_bigint)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(z, 0, sizeof (struct ra_bigint));
	if ((z->width = width)) {
		if (!(z->digits = malloc(z->width * sizeof (z->digits[0])))) {
			free(z);
			RA_TRACE("out of memory");
			return NULL;
		}
	}
	return z;
}

static struct ra_bigint *
clone(const struct ra_bigint *a)
{
	struct ra_bigint *z;

	if (!(z = allocate(a->width))) {
		RA_TRACE(NULL);
		return NULL;
	}
	z->sign = a->sign;
	memcpy(z->digits, a->digits, z->width * sizeof (z->digits[0]));
	return z;
}

static int
cmp(const struct ra_bigint *a, const struct ra_bigint *b)
{
	// different sign

	if (a->sign > b->sign) { // - ? +
		return -1;
	}
	if (a->sign < b->sign) { // + ? -
		return +1;
	}

	// different width

	if (a->width > b->width) {
		return a->sign ? -1 : +1;
	}
	if (a->width < b->width) {
		return a->sign ? +1 : -1;
	}

	// otherwise

	for (int i=a->width-1; i>=0; --i) {
		if (a->digits[i] > b->digits[i]) {
			return a->sign ? -1 : +1;
		}
		if (a->digits[i] < b->digits[i]) {
			return a->sign ? +1 : -1;
		}
	}
	return 0;
}

static struct ra_bigint *
uadd(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	int c;

	c = 0;
	if (!(z = allocate(RA_MAX(a->width, b->width) + 1))) {
		RA_TRACE(NULL);
		return NULL;
	}
	for (int i=0; i<z->width; ++i) {
		uint64_t a_ = (i < a->width) ? a->digits[i] : 0;
		uint64_t b_ = (i < b->width) ? b->digits[i] : 0;
		uint64_t z_;
		int c_;
		z_  = a_ + b_;
		c_  = (z_ < a_);
		z_ += c;
		c_ += (z_ < (uint64_t)c);
		z->digits[i] = z_;
		c = c_;
	}
	NORMALIZE(z);
	return z;
}

static struct ra_bigint *
usub(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	int c;

	c = 0;
	if (!(z = allocate(a->width))) {
		RA_TRACE(NULL);
		return NULL;
	}
	for (int i=0; i<z->width; ++i) {
		uint64_t a_ = a->digits[i];
		uint64_t b_ = (i < b->width) ? b->digits[i] : 0;
		uint64_t z_;
		int c_;
		z_  = a_ - b_;
		c_  = (a_ < b_);
		z_ -= c;
		c_ |= (z_ > a_);
		z->digits[i] = z_;
		c = c_;
	}
	NORMALIZE(z);
	return z;
}

static int
divmod(struct ra_bigint *a,
       struct ra_bigint *b,
       struct ra_bigint **q,
       struct ra_bigint **r)
{
	struct ra_bigint *q_, *r_, *b2, *q2;

	if (0 > cmp(a, b)) {
		(*q) = allocate(0);
		(*r) = clone(a);
		if (!(*q) || !(*r)) {
			ra_bigint_free(*q);
			ra_bigint_free(*r);
			RA_TRACE(NULL);
			return -1;
		}
	}
	else {
		q_ = r_ = NULL;
		if (!(b2 = ra_bigint_mul(b, &C[2])) ||
		    divmod(a, b2, &q_, &r_) ||
		    !(q2 = ra_bigint_mul(q_, &C[2]))) {
			ra_bigint_free(q_);
			ra_bigint_free(r_);
			ra_bigint_free(b2);
			RA_TRACE(NULL);
			return -1;
		}
		ra_bigint_free(b2);
		if (0 > cmp(r_, b)) {
			(*q) = q2;
			(*r) = r_;
			ra_bigint_free(q_);
		}
		else {
			(*q) = ra_bigint_add(q2, &C[1]);
			(*r) = ra_bigint_sub(r_, b);
			ra_bigint_free(q_);
			ra_bigint_free(r_);
			ra_bigint_free(q2);
			if (!(*q) || !(*r)) {
				RA_TRACE(NULL);
				return -1;
			}
		}
	}
	NORMALIZE((*q));
	NORMALIZE((*r));
	return 0;
}

ra_bigint_t
ra_bigint_init(const char *s)
{
	struct ra_bigint *a, *b, *z;
	int (*p2v)(int);
	int sign, m, v;

	if (!(z = allocate(0))) {
		RA_TRACE(NULL);
		return NULL;
	}
	if (s) {
		m = 10;
		sign = 0;
		p2v = dec2int;
		if ('-' == (*s)) {
			sign = 1;
			++s;
		}
		if (('0' == s[0]) && (('x' == s[1]) || ('X' == s[1]))) {
			p2v = hex2int;
			m = 16;
			++s;
			++s;
		}
		else if (('0' == s[0]) && (('b' == s[1]) || ('B' == s[1]))) {
			p2v = bin2int;
			m = 2;
			++s;
			++s;
		}
		while (*s) {
			if (0 > (v = p2v((unsigned char)(*s++)))) {
				ra_bigint_free(z);
				RA_TRACE("argument");
				return NULL;
			}
			if (!(a = ra_bigint_mul(z, &C[m])) ||
			    !(b = ra_bigint_add(a, &C[v]))) {
				ra_bigint_free(a);
				ra_bigint_free(z);
				RA_TRACE(NULL);
				return NULL;
			}
			ra_bigint_free(a);
			ra_bigint_free(z);
			z = b;
		}
		z->sign = sign;
	}
	return z;
}

ra_bigint_t
ra_bigint_add(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *z;
	int d;

	assert( a );
	assert( b );

	if (a->sign == b->sign) {
		if (!(z = uadd(a, b))) {
			RA_TRACE(NULL);
			return NULL;
		}
		z->sign = a->sign;
	}
	else {
		if (!(d = cmp(a, b))) {
			if (!(z = allocate(0))) {
				RA_TRACE(NULL);
				return NULL;
			}
		}
		else if (0 < d) {
			if (!(z = usub(a, b))) {
				RA_TRACE(NULL);
				return NULL;
			}
			z->sign = a->sign;
		}
		else {
			if (!(z = usub(b, a))) {
				RA_TRACE(NULL);
				return NULL;
			}
			z->sign = b->sign;
		}
	}
	return z;
}

ra_bigint_t
ra_bigint_sub(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *z;

	assert( a );
	assert( b );

	((struct ra_bigint *)b)->sign = !b->sign;
	z = ra_bigint_add(a, b);
	((struct ra_bigint *)b)->sign = !b->sign;
	if (!z) {
		RA_TRACE(NULL);
		return NULL;
	}
	return z;
}

ra_bigint_t
ra_bigint_mul(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *z;
	uint64_t h, l;

	assert( a );
	assert( b );

	if (!(z = allocate(a->width + b->width))) {
		RA_TRACE(NULL);
		return NULL;
	}
	SET(z, 0);
	for (int i=0; i<a->width; ++i) {
		for (int j=0; j<b->width; ++j) {
			mul(&h, &l, a->digits[i], b->digits[j]);
			z->digits[i + j] += l;
			if (z->digits[i + j] < l) {
				int k = i + j + 1;
				while(!(++z->digits[k++]));
			}
			z->digits[i + j + 1] += h;
			if (z->digits[i + j + 1] < h) {
				int k = i + j + 2;
				while(!(++z->digits[k++]));
			}
		}
	}
	z->sign = a->sign ^ b->sign;
	NORMALIZE(z);
	return z;
}

ra_bigint_t
ra_bigint_div(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *q, *r;

	assert( a );
	assert( b );

	if (ra_bigint_divmod(a, b, &q, &r)) {
		RA_TRACE(NULL);
		return NULL;
	}
	ra_bigint_free(r);
	return q;
}

ra_bigint_t
ra_bigint_mod(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *q, *r;

	assert( a );
	assert( b );

	if (ra_bigint_divmod(a, b, &q, &r)) {
		RA_TRACE(NULL);
		return NULL;
	}
	ra_bigint_free(q);
	return r;
}

int
ra_bigint_divmod(ra_bigint_t a, ra_bigint_t b, ra_bigint_t *q, ra_bigint_t *r)
{
	assert( a );
	assert( b );
	assert( q );
	assert( r );

	if (!b->width) {
		(*q) = (*r) = NULL;
		RA_TRACE("arithmetic");
		return -1;
	}
	if (IS_ONE(b)) {
		if (!((*q) = clone(a)) || !((*r) = allocate(0))) {
			ra_bigint_free(*q);
			RA_TRACE(NULL);
			return -1;
		}
		return 0;
	}
	if (divmod(a, b, q, r)) {
		RA_TRACE(NULL);
		return -1;
	}
	return 0;
}

void
ra_bigint_free(ra_bigint_t a)
{
	if (a) {
		free(a->digits);
		memset(a, 0, sizeof (struct ra_bigint));
	}
	free(a);
}

int
ra_bigint_cmp(ra_bigint_t a, ra_bigint_t b)
{
	assert( a );
	assert( b );

	return cmp(a, b);
}

int
ra_bigint_is_perfect_square(ra_bigint_t a)
{
	struct ra_bigint *t, *l, *h, *m;
	int d;

	assert( a );

	if (a->sign) {
		return 0;
	}
	if (!a->width) {
		return 1;
	}
	l = clone(&C[1]);
	h = ra_bigint_div(a, &C[2]);
	while (0 >= ra_bigint_cmp(l, h)) {
		t = ra_bigint_add(l, h);
		m = ra_bigint_div(t, &C[2]);
		ra_bigint_free(t);
		t = ra_bigint_mul(m, m);
		d = ra_bigint_cmp(t, a);
		ra_bigint_free(t);
		if (!d) {
			ra_bigint_free(l);
			ra_bigint_free(h);
			ra_bigint_free(m);
			return 1;
		}
		else if (0 > d) {
			ra_bigint_free(l);
			l = ra_bigint_add(m, &C[1]);
		}
		else {
			ra_bigint_free(h);
			h = ra_bigint_sub(m, &C[1]);
		}
		ra_bigint_free(m);
	}
	ra_bigint_free(h);
	ra_bigint_free(l);
	return 0;
}

int
ra_bigint_is_negative(ra_bigint_t a)
{
	assert( a );

	return a->sign ? 1 : 0;
}

int
ra_bigint_is_zero(ra_bigint_t a)
{
	assert( a );

	return a->width ? 0 : 1;
}

int
ra_bigint_is_one(ra_bigint_t a)
{
	assert( a );

	return IS_ONE(a);
}

int
ra_bigint_bist(void)
{
	ra_bigint_t a, b, t1, t2, t3, t4;

	// verify Fibonacci numbers

	if (!(a = ra_bigint_init("0")) || !(b = ra_bigint_init("1"))) {
		RA_TRACE(NULL);
		return -1;
	}
	for (int i=0; i<1000; ++i) {
		if (!(t1 = ra_bigint_add(a, b))) {
			ra_bigint_free(a);
			ra_bigint_free(b);
			RA_TRACE(NULL);
			return -1;
		}
		ra_bigint_free(a);
		a = b;
		b = t1;
		t1 = t2 = t3 = t4 = NULL;
		if (!(t1 = ra_bigint_mul(b, b)) ||
		    !(t2 = ra_bigint_mul(t1, &C[5])) ||
		    !(t3 = ra_bigint_add(t2, &C[4])) ||
		    !(t4 = ra_bigint_sub(t2, &C[4]))) {
			ra_bigint_free(a);
			ra_bigint_free(b);
			ra_bigint_free(t1);
			ra_bigint_free(t2);
			ra_bigint_free(t3);
			ra_bigint_free(t4);
			RA_TRACE(NULL);
			return -1;
		}
		if (!ra_bigint_is_perfect_square(t3) &&
		    !ra_bigint_is_perfect_square(t4)) {
			ra_bigint_free(a);
			ra_bigint_free(b);
			ra_bigint_free(t1);
			ra_bigint_free(t2);
			ra_bigint_free(t3);
			ra_bigint_free(t4);
			RA_TRACE("software");
			return -1;
		}
		ra_bigint_free(t1);
		ra_bigint_free(t2);
		ra_bigint_free(t3);
		ra_bigint_free(t4);
	}
	ra_bigint_free(a);
	ra_bigint_free(b);
	return 0;
}

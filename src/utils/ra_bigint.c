/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_bigint.c
 */

#include "ra_bigint.h"

#define MAX_DIGITS 1024

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
			(z)->width = 1;					\
			(z)->sign = 0;					\
		}							\
	} while (0)

#define IS_ZERO(a) ( (1 < (a)->width) || !(a)->digits[0] )

struct ra_bigint {
	int sign;
	int width;
	uint64_t *digits;
};

static uint64_t _C[]  = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

static const struct ra_bigint C[] = {
	{ 0,1, &_C[ 0] }, { 0,1, &_C[ 1] }, { 0,1, &_C[ 2] }, { 0,1, &_C[ 3] },
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

static struct ra_bigint *
allocate(int width)
{
	struct ra_bigint *bigint;

	assert( 0 < width );

	if (MAX_DIGITS < width) {
		RA_TRACE("integer too large");
		return NULL;
	}
	if (!(bigint = malloc(sizeof (struct ra_bigint)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(bigint, 0, sizeof (struct ra_bigint));
	bigint->width = width;
	bigint->digits = malloc(bigint->width * sizeof (bigint->digits[0]));
	if (!bigint->digits) {
		free(bigint);
		RA_TRACE("out of memory");
		return NULL;
	}
	return bigint;
}

static struct ra_bigint *
add(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	uint64_t a_, b_, z_;
	int i, c;

	assert( a->width <= b->width );

	if (!(z = allocate(RA_MAX(a->width, b->width) + 1))) {
		RA_TRACE(NULL);
		return NULL;
	}
	for (i=c=0; i<a->width; ++i) {
		a_ = a->digits[i];
		b_ = b->digits[i];
		z_ = a_ + b_ + c;
		c = (z_ < a_);
		z->digits[i] = z_;
	}
	for (; i<b->width; ++i) {
		b_ = b->digits[i];
		z_ = b_ + c;
		c = (!z_ && c);
		z->digits[i] = z_;
	}
	z->digits[i] = c;
	z->sign = 0;
	return z;
}

static struct ra_bigint *
sub(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	uint64_t a_, b_, z_;
	int i, c;

	assert( a->width <= b->width );
	assert( !a->sign || !b->sign );

	if (!(z = allocate(RA_MAX(a->width, b->width)))) {
		RA_TRACE(NULL);
		return NULL;
	}
	for (i=c=0; i<a->width; ++i) {
		a_ = a->digits[i];
		b_ = b->digits[i];
		z_ = a_ - b_ - c;
		if (a_ < (b_ + c)) {
			z_ = ~z_ + 1;
			c = 1;
		}
		else {
			c = 0;
		}
		z->digits[i] = z_;
	}
	for (; i<b->width; ++i) {
		b_ = b->digits[i];
		z_ = b_ - c;
		if (!b_ && c) {
			z_ = ~z_ + 1;
		}
		else {
			c = 0;
		}
		z->digits[i] = z_;
	}
	if ((z->sign = c)) {
		--z->digits[i - 1];
	}
	return z;
}

static struct ra_bigint *
addsub(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;

	/**
	 * n(a)/n(b) |a||b| operation
	 * ---------------------------
	 * +/+       <=      add(a, b)
	 * -/+       <=     -sub(a, b)
	 * +/-       <=      sub(a, b)
	 * -/-       <=     -add(a, b)
	 * +/+       >       add(b, a)
	 * -/+       >       sub(b, a)
	 * +/-       >      -sub(b, a)
	 * -/-       >      -add(b, a)
	 */

	if (a->width <= b->width) {
		if (a->sign ^ b->sign) {
			if (!(z = sub(a, b))) {
				RA_TRACE(NULL);
				return NULL;
			}
		}
		else {
			if (!(z = add(a, b))) {
				RA_TRACE(NULL);
				return NULL;
			}
		}
		if (a->sign) {
			z->sign = !z->sign;
		}
	}
	else {
		if (a->sign ^ b->sign) {
			if (!(z = sub(b, a))) {
				RA_TRACE(NULL);
				return NULL;
			}
		}
		else {
			if (!(z = add(b, a))) {
				RA_TRACE(NULL);
				return NULL;
			}
		}
		if (b->sign) {
			z->sign = !z->sign;
		}
	}
	NORMALIZE(z);
	return z;
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

ra_bigint_t
ra_bigint_init(const char *s)
{
	struct ra_bigint *a, *b, *z;
	int (*p2v)(int);
	int sign, m, v;

	if (!(z = allocate(1))) {
		RA_TRACE(NULL);
		return NULL;
	}
	SET(z, 0);
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
			if (!(a = ra_bigint_mul(z, (ra_bigint_t)&C[m])) ||
			    !(b = ra_bigint_add(a, (ra_bigint_t)&C[v]))) {
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

	assert( a && a->digits );
	assert( b && b->digits );

	if (!(z = addsub(a, b))) {
		RA_TRACE(NULL);
		return NULL;
	}
	return z;
}

ra_bigint_t
ra_bigint_sub(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *z;

	assert( a && a->digits );
	assert( b && b->digits );

	b->sign = !b->sign;
	if (!(z = addsub(a, b))) {
		b->sign = !b->sign;
		RA_TRACE(NULL);
		return NULL;
	}
	b->sign = !b->sign;
	return z;
}

ra_bigint_t
ra_bigint_mul(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *z;
	uint64_t h, l;

	assert( a && a->digits );
	assert( b && b->digits );

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

int
ra_bigint_divmod(ra_bigint_t a, ra_bigint_t b, ra_bigint_t *q, ra_bigint_t *r)
{
	assert( a && a->width );
	assert( b && b->width );
	assert( q );
	assert( r );

	(*q) = (*r) = NULL;
	if (IS_ZERO(b)) {
		RA_TRACE("divide by zero");
		return -1;
	}
	if (!((*q) = allocate(RA_MAX(1, a->width - b->width))) ||
	    !((*r) = allocate(b->width))) {
		ra_bigint_free(*q);
		RA_TRACE(NULL);
		return -1;
	}
	SET((*q), 0);
	SET((*r), 0);

	NORMALIZE((*q));
	NORMALIZE((*r));
	return 0;
}

void
ra_bigint_free(ra_bigint_t bigint)
{
	if (bigint) {
		free(bigint->digits);
		memset(bigint, 0, sizeof (struct ra_bigint));
	}
	free(bigint);
}

int
ra_bigint_bist(void)
{
	return 0;
}

static void print(struct ra_bigint *bigint, const char *name) {
	ra_log("%s: (%s)", name, bigint->sign ? "-" : "");
	for (int i=0; i<bigint->width; ++i) {
		ra_log("  %lx", (unsigned long)bigint->digits[i]);
	}
}

void x(void) {
	ra_bigint_t a = ra_bigint_init("0b1111");
	print(a, "a");

	ra_bigint_t b = ra_bigint_init("0xff");
	print(b, "b");

	ra_bigint_t q, r;
	if (!ra_bigint_divmod(a, b, &q, &r)) {
		print(q, "q");
		print(r, "r");
	}
	ra_bigint_free(a);
	ra_bigint_free(b);
	ra_bigint_free(q);
	ra_bigint_free(r);
}

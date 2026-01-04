/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_kernel.h"
#include "ra_bigint.h"

#define MAX_PARTS 16384

#define IS_ONE(a)      ( (1 == a->width) && (1 == a->parts[0]) )
#define IS_ZERO(a)     ( (a)->width ? 0 : 1 )
#define IS_NEGATIVE(a) ( (a)->sign )

struct ra_bigint {
	int sign;
	int width;
	uint64_t *parts;
};

static uint64_t C_[17]  = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

static struct ra_bigint C__[17] = {
	{ 0,0, NULL    }, { 0,1, &C_[ 1] }, { 0,1, &C_[ 2] }, { 0,1, &C_[ 3] },
	{ 0,1, &C_[ 4] }, { 0,1, &C_[ 5] }, { 0,1, &C_[ 6] }, { 0,1, &C_[ 7] },
	{ 0,1, &C_[ 8] }, { 0,1, &C_[ 9] }, { 0,1, &C_[10] }, { 0,1, &C_[11] },
	{ 0,1, &C_[12] }, { 0,1, &C_[13] }, { 0,1, &C_[14] }, { 0,1, &C_[15] },
	{ 0,1, &C_[16] }
};

const ra_bigint_t RA_BIGINT_CONST[17] = {
	&C__[ 0], &C__[ 1], &C__[ 2], &C__[ 3], &C__[ 4],
	&C__[ 5], &C__[ 6], &C__[ 7], &C__[ 8], &C__[ 9],
	&C__[10], &C__[11], &C__[12], &C__[13], &C__[14],
	&C__[15], &C__[16]
};

static void
destroy(struct ra_bigint *z)
{
	if (z) {
		RA_FREE(z->parts);
		memset(z, 0, sizeof (struct ra_bigint));
	}
	RA_FREE(z);
}

static struct ra_bigint *
create(int width)
{
	struct ra_bigint *z;

	if (MAX_PARTS < width) {
		RA_TRACE("integer exceeds maximum limit");
		return NULL;
	}
	if (!(z = malloc(sizeof (struct ra_bigint)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(z, 0, sizeof (struct ra_bigint));
	if ((z->width = width)) {
		if (!(z->parts = malloc(z->width * sizeof (z->parts[0])))) {
			destroy(z);
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

	if (!(z = create(a->width))) {
		RA_TRACE("^");
		return NULL;
	}
	z->sign = a->sign;
	memcpy(z->parts, a->parts, z->width * sizeof (z->parts[0]));
	return z;
}

static void
normalize(struct ra_bigint *z)
{
	while (z->width && !z->parts[z->width - 1]) {
		--z->width;
	}
	if (!z->width) {
		RA_FREE(z->parts);
		z->parts = NULL;
		z->sign = 0;
	}
}

static int
cmp(const struct ra_bigint *a, const struct ra_bigint *b)
{
	int i;

	if (a->sign > b->sign) {
		return -1;
	}
	if (a->sign < b->sign) {
		return +1;
	}
	if (a->width > b->width) {
		return a->sign ? -1 : +1;
	}
	if (a->width < b->width) {
		return a->sign ? +1 : -1;
	}
	for (i=a->width-1; i>=0; --i) {
		if (a->parts[i] > b->parts[i]) {
			return a->sign ? -1 : +1;
		}
		if (a->parts[i] < b->parts[i]) {
			return a->sign ? +1 : -1;
		}
	}
	return 0;
}

static struct ra_bigint *
uadd(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	int i, c;

	c = 0;
	if (!(z = create(RA_MAX(a->width, b->width) + 1))) {
		RA_TRACE("^");
		return NULL;
	}
	for (i=0; i<z->width; ++i) {
		uint64_t a_ = (i < a->width) ? a->parts[i] : 0;
		uint64_t b_ = (i < b->width) ? b->parts[i] : 0;
		uint64_t z_;
		int c_;
		z_  = a_ + b_;
		c_  = (z_ < a_);
		z_ += c;
		c_ += (z_ < (uint64_t)c);
		z->parts[i] = z_;
		c = c_;
	}
	normalize(z);
	return z;
}

static struct ra_bigint *
usub(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	int i, c;

	c = 0;
	if (!(z = create(a->width))) {
		RA_TRACE("^");
		return NULL;
	}
	for (i=0; i<z->width; ++i) {
		uint64_t a_ = a->parts[i];
		uint64_t b_ = (i < b->width) ? b->parts[i] : 0;
		uint64_t z_;
		int c_;
		z_  = a_ - b_;
		c_  = (a_ < b_);
		z_ -= c;
		c_ |= (z_ > a_);
		z->parts[i] = z_;
		c = c_;
	}
	normalize(z);
	return z;
}

static struct ra_bigint *
add(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	int d;

	if (a->sign == b->sign) {
		if (!(z = uadd(a, b))) {
			RA_TRACE("^");
			return NULL;
		}
		z->sign = a->sign;
	}
	else {
		if (!(d = cmp(a, b))) {
			if (!(z = create(0))) {
				RA_TRACE("^");
				return NULL;
			}
		}
		else if (0 < d) {
			if (!(z = usub(a, b))) {
				RA_TRACE("^");
				return NULL;
			}
			z->sign = a->sign;
		}
		else {
			if (!(z = usub(b, a))) {
				RA_TRACE("^");
				return NULL;
			}
			z->sign = b->sign;
		}
	}
	return z;
}

static struct ra_bigint *
sub(const struct ra_bigint *a, const struct ra_bigint *b_)
{
	struct ra_bigint *z, b;

	b = (*b_);
	b.sign = b_->sign ? 0 : 1;
	if (!(z = add(a, &b))) {
		RA_TRACE("^");
		return NULL;
	}
	return z;
}

static void
mul128(uint64_t *h, uint64_t *l, uint64_t a, uint64_t b)
{
#ifdef __SIZEOF_INT128__
	__uint128_t t = (__uint128_t)a * (__uint128_t)b;
	(*h) = (uint64_t)(t >> 64);
	(*l) = (uint64_t)t;
#else
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
		t4 += 1LLU << 32;
	}
	t4 += t3 >> 32;
	(*h) = t4;
	(*l) = (t3 << 32) | (t1 & 0xffffffff);
#endif /* __SIZEOF_INT128__ */
}

static struct ra_bigint *
mul(const struct ra_bigint *a, const struct ra_bigint *b)
{
	struct ra_bigint *z;
	uint64_t h, l;
	int i, j;

	if (!(z = create(a->width + b->width))) {
		RA_TRACE("^");
		return NULL;
	}
	memset(z->parts, 0, z->width * sizeof (z->parts[0]));
	for (i=0; i<a->width; ++i) {
		for (j=0; j<b->width; ++j) {
			mul128(&h, &l, a->parts[i], b->parts[j]);
			z->parts[i + j] += l;
			if (z->parts[i + j] < l) {
				int k = i + j + 1;
				while(!(++z->parts[k++]));
			}
			z->parts[i + j + 1] += h;
			if (z->parts[i + j + 1] < h) {
				int k = i + j + 2;
				while(!(++z->parts[k++]));
			}
		}
	}
	z->sign = a->sign ^ b->sign;
	normalize(z);
	return z;
}

static int
slow_divmod(const struct ra_bigint *a,
	    const struct ra_bigint *b,
	    struct ra_bigint **q,
	    struct ra_bigint **r)
{
	struct ra_bigint *q_, *r_, *b2, *q2;

	if (0 > cmp(a, b)) {
		if (!((*q) = create(0)) || !((*r) = clone(a))) {
			destroy(*q);
			destroy(*r);
			RA_TRACE("^");
			return -1;
		}
	}
	else {
		q_ = r_ = NULL;
		if (!(b2 = mul(b, RA_BIGINT_CONST[2])) ||
		    slow_divmod(a, b2, &q_, &r_) ||
		    !(q2 = mul(q_, RA_BIGINT_CONST[2]))) {
			destroy(q_);
			destroy(r_);
			destroy(b2);
			RA_TRACE("^");
			return -1;
		}
		destroy(b2);
		if (0 > cmp(r_, b)) {
			(*q) = q2;
			(*r) = r_;
			destroy(q_);
		}
		else {
			(*q) = add(q2, RA_BIGINT_CONST[1]);
			(*r) = sub(r_, b);
			destroy(q_);
			destroy(r_);
			destroy(q2);
			if (!(*q) || !(*r)) {
				RA_TRACE("^");
				return -1;
			}
		}
	}
	normalize(*q);
	normalize(*r);
	return 0;
}

static int
divmod(const struct ra_bigint *a_,
       const struct ra_bigint *b_,
       struct ra_bigint **q,
       struct ra_bigint **r)
{
	struct ra_bigint a, b;

	if (IS_ZERO(b_)) {
		RA_TRACE("divide by zero");
		return -1;
	}
	a = (*a_);
	b = (*b_);
	a.sign = 0;
	b.sign = 0;
	if (slow_divmod(&a, &b, q, r)) {
		RA_TRACE("^");
		return -1;
	}
	if (IS_NEGATIVE(a_) ^ IS_NEGATIVE(b_)) {
		(*q)->sign = 1;
	}
	if (IS_NEGATIVE(a_)) {
		(*r)->sign = 1;
	}
	return 0;
}

static int
count_bits(const struct ra_bigint *a)
{
	int i;

	if (0 > (i = a->width - 1)) {
		return 1;
	}
	return i * 64 + (64 - __builtin_clzll(a->parts[i]));
}

static int
count_digits(const struct ra_bigint *a)
{
	return (int)ceil(count_bits(a) * log(2) / log(10));
}

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
convert_int(int64_t v)
{
	struct ra_bigint *z;

	if (!(z = create(1))) {
		RA_TRACE("^");
		return NULL;
	}
	z->parts[0] = (0 > v) ? ~(uint64_t)v + 1 : (uint64_t)v;
	z->sign = (0 > v) ? 1 : 0;
	normalize(z);
	return z;
}

static struct ra_bigint *
convert_real(double r)
{
	struct ra_bigint *z;
	uint64_t mantissa;
	int i, exp, sign;

	/* handle corner cases, simplifying the main conversion process */

	if (isnan(r)) {
		RA_TRACE("invalid NaN real value");
		return NULL;
	}
	else if (isinf(r)) {
		RA_TRACE("invalid INF real value");
		return NULL;
	}

	/* handle small values to avoid IEEE denormalization cases */

	if ((LONG_MIN < r) && (LONG_MAX > r)) {
		if (!(z = convert_int(lround(r)))) {
			RA_TRACE("^");
			return NULL;
		}
		return z;
	}

	/* capture the sign */

	sign = 0;
	if (0.0 > r) {
		sign = 1;
		r = fabs(r);
	}

	/* exponent and mantissa, adding the implicit 53rd bit to mantissa */

	r = frexp(r, &exp);
	memcpy(&mantissa, &r, sizeof (mantissa));
	mantissa &= 0xfffffffffffff;
	mantissa |= 0x10000000000000;

	/* sanity check */

	assert( (64 < exp) && (1024 > exp) );

	/* convert */

	if (!(z = create(RA_DUP(exp, 64)))) {
		RA_TRACE("^");
		return NULL;
	}

	/* trailing zero parts */

	i = 0;
	exp -= 53; /* this many significant bits */
	while (64 <= exp) {
		exp -= 64;
		z->parts[i++] = 0;
	}
	exp += 53;

	/* significant bits */

	z->parts[i++] = mantissa << (exp - 53);
	if (i < z->width) {
		z->parts[i++] = mantissa >> (117 - exp);
	}
	z->sign = sign;
	normalize(z);
	return z;
}

struct ra_bigint *
convert_string(const char *s)
{
	struct ra_bigint *a, *b, *z;
	int (*p2v)(int);
	const char *e, *s_;
	double r;
	int v, m;

	m = 10;
	p2v = dec2int;
	if (('0' == s[0]) && (('x' == s[1]) || ('X' == s[1]))) {
		m = 16;
		p2v = hex2int;
		s += 2;
	}
	else if (('0' == s[0]) && (('b' == s[1]) || ('B' == s[1]))) {
		m = 2;
		p2v = bin2int;
		s += 2;
	}
	else {
		errno = 0;
		r = strtod(s, (char **)&e);
		if ((EINVAL == errno) || (ERANGE == errno)) {
			RA_TRACE("invalid real value");
			return NULL;
		}
		s_ = s;
		while (s < e) {
			if (('.' == (*s)) || ('e' == (*s)) || ('E' == (*s))) {
				return convert_real(r);
			}
			++s;
		}
		s = s_;
	}
	if (0 > (v = p2v((unsigned char)(*s)))) {
		RA_TRACE("invalid integer value");
		return NULL;
	}
	if (!(z = convert_int(0))) {
		RA_TRACE("^");
		return NULL;
	}
	while (*s) {
		if (0 > (v = p2v((unsigned char)(*s)))) {
			break;
		}
		if (!(a = mul(z, RA_BIGINT_CONST[m])) ||
		    !(b = add(a, RA_BIGINT_CONST[v]))) {
			destroy(a);
			destroy(z);
			RA_TRACE("^");
			return NULL;
		}
		destroy(a);
		destroy(z);
		z = b;
		++s;
	}
	return z;
}

static const char *
print(const struct ra_bigint *a)
{
	static uint64_t M_[] = { 1000000000000000000LU };
	const struct ra_bigint M = { 0, 1, (uint64_t *)M_ };
	struct ra_bigint *q, *r, *q_;
	uint64_t *stack;
	size_t i, len;
	char *buf;

	len = count_digits(a) + 2;
	if (!(buf = malloc(len)) ||
	    !(stack = malloc(RA_DUP(len, 18) * sizeof (stack[0])))) {
		RA_FREE(buf);
		RA_TRACE("out of memory");
		return NULL;
	}
	if (!(q = clone(a))) {
		RA_FREE(buf);
		RA_FREE(stack);
		RA_TRACE("^");
		return NULL;
	}
	i = 0;
	while (!IS_ZERO(q)) {
		if (divmod(q, &M, &q_, &r)) {
			destroy(q);
			RA_FREE(buf);
			RA_FREE(stack);
			RA_TRACE("^");
			return NULL;
		}
		stack[i++] = r->width ? r->parts[0] : 0;
		destroy(r);
		destroy(q);
		q = q_;
	}
	destroy(q);
	ra_sprintf(buf, len, "0");
	if (i) {
		ra_sprintf(buf,
			   len,
			   "%s%lu",
			   IS_NEGATIVE(a) ? "-" : "",
			   (unsigned long)stack[--i]);
		while (i) {
			ra_sprintf(buf + strlen(buf),
				   len - strlen(buf),
				   "%018lu",
				   (unsigned long)stack[--i]);
		}
	}
	RA_FREE(stack);
	return buf;
}

static int
is_square(const struct ra_bigint *a)
{
	struct ra_bigint *t, *l, *h, *m;
	int d;

	if (IS_NEGATIVE(a)) {
		return 0;
	}
	if (IS_ZERO(a)) {
		return 1;
	}
	if (!(l = clone(RA_BIGINT_CONST[1])) ||
	    !(h = ra_bigint_div((ra_bigint_t)a, RA_BIGINT_CONST[2]))) {
		destroy(l);
		RA_TRACE("^");
		return -1;
	}
	while (0 >= cmp(l, h)) {
		if (!(t = add(l, h)) ||
		    !(m = ra_bigint_div(t, RA_BIGINT_CONST[2]))) {
			destroy(t);
			destroy(l);
			destroy(h);
			RA_TRACE("^");
			return -1;
		}
		destroy(t);
		if (!(t = mul(m, m))) {
			destroy(m);
			destroy(l);
			destroy(h);
			RA_TRACE("^");
			return -1;
		}
		d = cmp(t, a);
		destroy(t);
		if (!d) {
			destroy(l);
			destroy(h);
			destroy(m);
			return 1;
		}
		else if (0 > d) {
			destroy(l);
			if (!(l = add(m, RA_BIGINT_CONST[1]))) {
				destroy(m);
				destroy(h);
				RA_TRACE("^");
				return -1;
			}
		}
		else {
			destroy(h);
			if (!(h = sub(m, RA_BIGINT_CONST[1]))) {
				destroy(m);
				destroy(l);
				RA_TRACE("^");
				return -1;
			}
		}
		destroy(m);
	}
	destroy(h);
	destroy(l);
	return 0;
}

void
ra_bigint_free(ra_bigint_t z)
{
	destroy(z);
}

ra_bigint_t
ra_bigint_int(int64_t v)
{
	return convert_int(v);
}

ra_bigint_t
ra_bigint_real(double r)
{
	return convert_real(r);
}

ra_bigint_t
ra_bigint_string(const char *s)
{
	assert( s && strlen(s) );

	return convert_string(s);
}

const char *
ra_bigint_print(ra_bigint_t a)
{
	return print(a);
}

ra_bigint_t
ra_bigint_add(ra_bigint_t a, ra_bigint_t b)
{
	return add(a, b);
}

ra_bigint_t
ra_bigint_sub(ra_bigint_t a, ra_bigint_t b)
{
	return sub(a, b);
}

ra_bigint_t
ra_bigint_mul(ra_bigint_t a, ra_bigint_t b)
{
	return mul(a, b);
}

ra_bigint_t
ra_bigint_div(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *q, *r;

	if (divmod(a, b, &q, &r)) {
		RA_TRACE("^");
		return NULL;
	}
	destroy(r);
	return q;
}

ra_bigint_t
ra_bigint_mod(ra_bigint_t a, ra_bigint_t b)
{
	struct ra_bigint *q, *r;

	if (divmod(a, b, &q, &r)) {
		RA_TRACE("^");
		return NULL;
	}
	destroy(q);
	return r;
}

int
ra_bigint_divmod(ra_bigint_t a, ra_bigint_t b, ra_bigint_t *r, ra_bigint_t *q)
{
	return divmod(a, b, r, q);
}

int
ra_bigint_cmp(ra_bigint_t a, ra_bigint_t b)
{
	return cmp(a, b);
}

int
ra_bigint_bits(ra_bigint_t a)
{
	return count_bits(a);
}

int
ra_bigint_digits(ra_bigint_t a)
{
	return count_digits(a);
}

int
ra_bigint_is_zero(ra_bigint_t a)
{
	return IS_ZERO(a);
}

int
ra_bigint_is_one(ra_bigint_t a)
{
	return IS_ONE(a);
}

int
ra_bigint_is_square(ra_bigint_t a)
{
	return is_square(a);
}

int
ra_bigint_is_negative(ra_bigint_t a)
{
	return IS_NEGATIVE(a);
}

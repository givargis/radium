/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_ec.h"

static uint8_t G_H[256][16];
static uint8_t G_L[256][16];
static uint8_t A_H[256][16];
static uint8_t A_L[256][16];
static uint8_t B_H[256][256][16];
static uint8_t B_L[256][256][16];

#define U64(x) ( (uint64_t)(x) )

static uint8_t
mul(uint8_t a, uint8_t b)
{
	uint8_t p;

	p = 0;
	while (a && b) {
		if (b & 1) {
			p ^= a;
		}
		if (a & 0x80) {
			a = (a << 1) ^ 0x11d;
		}
		else {
			a <<= 1;
		}
		b >>= 1;
	}
	return p;
}

static uint8_t
power(uint8_t a, int b)
{
	uint8_t p;

	p = 1;
	b %= 255;
	if (b < 0) {
		b += 255;
	}
	while (b) {
		if (b & 1) {
			p = mul(p, a);
		}
		a = mul(a, a);
		b >>= 1;
	}
	return p;
}

void
ra_ec_init(void)
{
	uint8_t g, a, b;
	int i, j, x, y;

	/**
	 * G(j) : {02}^j
	 */

	for (j=0; j<256; ++j) {
		g = power(2, j);
		for (i=0; i<16; ++i) {
			G_H[j][i] = mul(g, (uint8_t)(i << 4));
			G_L[j][i] = mul(g, (uint8_t)(i & 15));
		}
	}

	/**
	 * A'(x,y) : A(y - x) | y > x
	 * A(j)    : {02}^j * ( {02}^j + {01} )^-1
	 */

	for (j=1; j<256; ++j) {
		g = power(2, j);
		a = mul(g, power(g^1, 254));
		for (i=0; i<16; ++i) {
			A_H[j][i] = mul(a, (uint8_t)(i << 4));
			A_L[j][i] = mul(a, (uint8_t)(i & 15));
		}
	}

	/**
	 * B(x,y) : {02}^-x * ( {02}^(y-x) + {01} )^-1 | y > x
	 */

	for (x=0; x<256; ++x) {
		for (y=x+1; y<256; ++y) {
			g = power(2, y - x);
			b = mul(power(2, 255 - x), power(g^1, 254));
			for (i=0; i<16; ++i) {
				B_H[x][y][i] = mul(b, (uint8_t)(i << 4));
				B_L[x][y][i] = mul(b, (uint8_t)(i & 15));
			}
		}
	}
}

void
ra_ec_encode_pq(void *buf, int k, int n)
{
	uint64_t d, *p, *q;
	int i, j;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA_EC_MIN_K <= k) && (RA_EC_MAX_K >= k) );
	assert( (RA_EC_MIN_N <= n) && (RA_EC_MAX_N >= n) );

	p = RA_EC_P(buf, k, n);
	q = RA_EC_Q(buf, k, n);
	memset(p, 0, n);
	memset(q, 0, n);
	for (j=0; j<k; ++j) {
		for (i=0; i<(n/8); ++i) {
			d = RA_EC_D(buf, j, n)[i];
			p[i] ^= d;
			q[i] ^= ((U64(G_H[j][d >> 60 & 15]) << 56 |
				  U64(G_H[j][d >> 52 & 15]) << 48 |
				  U64(G_H[j][d >> 44 & 15]) << 40 |
				  U64(G_H[j][d >> 36 & 15]) << 32 |
				  U64(G_H[j][d >> 28 & 15]) << 24 |
				  U64(G_H[j][d >> 20 & 15]) << 16 |
				  U64(G_H[j][d >> 12 & 15]) <<  8 |
				  U64(G_H[j][d >>  4 & 15])) ^
				 (U64(G_L[j][d >> 56 & 15]) << 56 |
				  U64(G_L[j][d >> 48 & 15]) << 48 |
				  U64(G_L[j][d >> 40 & 15]) << 40 |
				  U64(G_L[j][d >> 32 & 15]) << 32 |
				  U64(G_L[j][d >> 24 & 15]) << 24 |
				  U64(G_L[j][d >> 16 & 15]) << 16 |
				  U64(G_L[j][d >>  8 & 15]) <<  8 |
				  U64(G_L[j][d >>  0 & 15])));
		}
	}
}

void
ra_ec_encode_p(void *buf, int k, int n)
{
	uint64_t *p;
	int i, j;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA_EC_MIN_K <= k) && (RA_EC_MAX_K >= k) );
	assert( (RA_EC_MIN_N <= n) && (RA_EC_MAX_N >= n) );

	p = RA_EC_P(buf, k, n);
	memset(p, 0, n);
	for (j=0; j<k; ++j) {
		for (i=0; i<(n/8); ++i) {
			p[i] ^= RA_EC_D(buf, j, n)[i];
		}
	}
}

void
ra_ec_encode_q(void *buf, int k, int n)
{
	uint64_t d, *q;
	int i, j;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA_EC_MIN_K <= k) && (RA_EC_MAX_K >= k) );
	assert( (RA_EC_MIN_N <= n) && (RA_EC_MAX_N >= n) );

	q = RA_EC_Q(buf, k, n);
	memset(q, 0, n);
	for (j=0; j<k; ++j) {
		for (i=0; i<(n/8); ++i) {
			d = RA_EC_D(buf, j, n)[i];
			q[i] ^= ((U64(G_H[j][d >> 60 & 15]) << 56 |
				  U64(G_H[j][d >> 52 & 15]) << 48 |
				  U64(G_H[j][d >> 44 & 15]) << 40 |
				  U64(G_H[j][d >> 36 & 15]) << 32 |
				  U64(G_H[j][d >> 28 & 15]) << 24 |
				  U64(G_H[j][d >> 20 & 15]) << 16 |
				  U64(G_H[j][d >> 12 & 15]) <<  8 |
				  U64(G_H[j][d >>  4 & 15])) ^
				 (U64(G_L[j][d >> 56 & 15]) << 56 |
				  U64(G_L[j][d >> 48 & 15]) << 48 |
				  U64(G_L[j][d >> 40 & 15]) << 40 |
				  U64(G_L[j][d >> 32 & 15]) << 32 |
				  U64(G_L[j][d >> 24 & 15]) << 24 |
				  U64(G_L[j][d >> 16 & 15]) << 16 |
				  U64(G_L[j][d >>  8 & 15]) <<  8 |
				  U64(G_L[j][d >>  0 & 15])));
		}
	}
}

void
ra_ec_encode_dp(void *buf, int k, int n, int x_)
{
	uint64_t *d, *p;
	int i, j;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA_EC_MIN_K <= k) && (RA_EC_MAX_K >= k) );
	assert( (RA_EC_MIN_N <= n) && (RA_EC_MAX_N >= n) );
	assert( (0 <= x_) && (k > x_) );

	p = RA_EC_P(buf, k, n);
	d = RA_EC_D(buf, x_, n);
	memcpy(d, p, n);
	for (j=0; j<k; ++j) {
		if (j != x_) {
			for (i=0; i<(n/8); ++i) {
				d[i] ^= RA_EC_D(buf, j, n)[i];
			}
		}
	}
}

void
ra_ec_encode_dq(void *buf, int k, int n, int x_)
{
	uint64_t d, *q, *x;
	int i, j;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (0 < k) && (256 > k) );
	assert( (0 < n) && (0 == (n % 8)) );
	assert( (RA_EC_MIN_K <= k) && (RA_EC_MAX_K >= k) );
	assert( (RA_EC_MIN_N <= n) && (RA_EC_MAX_N >= n) );
	assert( (0 <= x_) && (k > x_) );

	q = RA_EC_Q(buf, k, n);
	x = RA_EC_D(buf, x_, n);
	memcpy(x, q, n);
	for (j=0; j<k; ++j) {
		if (j != x_) {
			for (i=0; i<(n/8); ++i) {
				d = RA_EC_D(buf, j, n)[i];
				x[i] ^= ((U64(G_H[j][d >> 60 & 15]) << 56 |
					  U64(G_H[j][d >> 52 & 15]) << 48 |
					  U64(G_H[j][d >> 44 & 15]) << 40 |
					  U64(G_H[j][d >> 36 & 15]) << 32 |
					  U64(G_H[j][d >> 28 & 15]) << 24 |
					  U64(G_H[j][d >> 20 & 15]) << 16 |
					  U64(G_H[j][d >> 12 & 15]) <<  8 |
					  U64(G_H[j][d >>  4 & 15])) ^
					 (U64(G_L[j][d >> 56 & 15]) << 56 |
					  U64(G_L[j][d >> 48 & 15]) << 48 |
					  U64(G_L[j][d >> 40 & 15]) << 40 |
					  U64(G_L[j][d >> 32 & 15]) << 32 |
					  U64(G_L[j][d >> 24 & 15]) << 24 |
					  U64(G_L[j][d >> 16 & 15]) << 16 |
					  U64(G_L[j][d >>  8 & 15]) <<  8 |
					  U64(G_L[j][d >>  0 & 15])));
			}
		}
	}
	x_ = 255 - x_;
	for (i=0; i<(n/8); ++i) {
		d = x[i];
		x[i] = ((U64(G_H[x_][d >> 60 & 15]) << 56 |
			 U64(G_H[x_][d >> 52 & 15]) << 48 |
			 U64(G_H[x_][d >> 44 & 15]) << 40 |
			 U64(G_H[x_][d >> 36 & 15]) << 32 |
			 U64(G_H[x_][d >> 28 & 15]) << 24 |
			 U64(G_H[x_][d >> 20 & 15]) << 16 |
			 U64(G_H[x_][d >> 12 & 15]) <<  8 |
			 U64(G_H[x_][d >>  4 & 15])) ^
			(U64(G_L[x_][d >> 56 & 15]) << 56 |
			 U64(G_L[x_][d >> 48 & 15]) << 48 |
			 U64(G_L[x_][d >> 40 & 15]) << 40 |
			 U64(G_L[x_][d >> 32 & 15]) << 32 |
			 U64(G_L[x_][d >> 24 & 15]) << 24 |
			 U64(G_L[x_][d >> 16 & 15]) << 16 |
			 U64(G_L[x_][d >>  8 & 15]) <<  8 |
			 U64(G_L[x_][d >>  0 & 15])));
	}
}

void
ra_ec_encode_dd(void *buf, int k, int n, int x_, int y_)
{
	uint64_t d, d1, d2, *p, *q, *x, *y;
	int i, j;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA_EC_MIN_K <= k) && (RA_EC_MAX_K >= k) );
	assert( (RA_EC_MIN_N <= n) && (RA_EC_MAX_N >= n) );
	assert( x_ < y_ );
	assert( (0 <= x_) && (k > x_) );
	assert( (0 <= y_) && (k > y_) );

	p = RA_EC_P(buf, k, n);
	q = RA_EC_Q(buf, k, n);
	x = RA_EC_D(buf, x_, n);
	y = RA_EC_D(buf, y_, n);
	memcpy(x, q, n);
	memcpy(y, p, n);
	for (j=0; j<k; ++j) {
		if ((j != x_) && (j != y_)) {
			for (i=0; i<(n/8); ++i) {
				d = RA_EC_D(buf, j, n)[i];
				y[i] ^= d;
				x[i] ^= ((U64(G_H[j][d >> 60 & 15]) << 56 |
					  U64(G_H[j][d >> 52 & 15]) << 48 |
					  U64(G_H[j][d >> 44 & 15]) << 40 |
					  U64(G_H[j][d >> 36 & 15]) << 32 |
					  U64(G_H[j][d >> 28 & 15]) << 24 |
					  U64(G_H[j][d >> 20 & 15]) << 16 |
					  U64(G_H[j][d >> 12 & 15]) <<  8 |
					  U64(G_H[j][d >>  4 & 15])) ^
					 (U64(G_L[j][d >> 56 & 15]) << 56 |
					  U64(G_L[j][d >> 48 & 15]) << 48 |
					  U64(G_L[j][d >> 40 & 15]) << 40 |
					  U64(G_L[j][d >> 32 & 15]) << 32 |
					  U64(G_L[j][d >> 24 & 15]) << 24 |
					  U64(G_L[j][d >> 16 & 15]) << 16 |
					  U64(G_L[j][d >>  8 & 15]) <<  8 |
					  U64(G_L[j][d >>  0 & 15])));
			}
		}
	}
	for (i=0; i<(n/8); ++i) {
		d1 = x[i];
		d2 = y[i];
		x[i] = ((U64(A_H[y_ - x_][d2 >> 60 & 15]) << 56 |
			 U64(A_H[y_ - x_][d2 >> 52 & 15]) << 48 |
			 U64(A_H[y_ - x_][d2 >> 44 & 15]) << 40 |
			 U64(A_H[y_ - x_][d2 >> 36 & 15]) << 32 |
			 U64(A_H[y_ - x_][d2 >> 28 & 15]) << 24 |
			 U64(A_H[y_ - x_][d2 >> 20 & 15]) << 16 |
			 U64(A_H[y_ - x_][d2 >> 12 & 15]) <<  8 |
			 U64(A_H[y_ - x_][d2 >>  4 & 15])) ^
			(U64(A_L[y_ - x_][d2 >> 56 & 15]) << 56 |
			 U64(A_L[y_ - x_][d2 >> 48 & 15]) << 48 |
			 U64(A_L[y_ - x_][d2 >> 40 & 15]) << 40 |
			 U64(A_L[y_ - x_][d2 >> 32 & 15]) << 32 |
			 U64(A_L[y_ - x_][d2 >> 24 & 15]) << 24 |
			 U64(A_L[y_ - x_][d2 >> 16 & 15]) << 16 |
			 U64(A_L[y_ - x_][d2 >>  8 & 15]) <<  8 |
			 U64(A_L[y_ - x_][d2 >>  0 & 15]))) ^
			((U64(B_H[x_][y_][d1 >> 60 & 15]) << 56 |
			  U64(B_H[x_][y_][d1 >> 52 & 15]) << 48 |
			  U64(B_H[x_][y_][d1 >> 44 & 15]) << 40 |
			  U64(B_H[x_][y_][d1 >> 36 & 15]) << 32 |
			  U64(B_H[x_][y_][d1 >> 28 & 15]) << 24 |
			  U64(B_H[x_][y_][d1 >> 20 & 15]) << 16 |
			  U64(B_H[x_][y_][d1 >> 12 & 15]) <<  8 |
			  U64(B_H[x_][y_][d1 >>  4 & 15])) ^
			 (U64(B_L[x_][y_][d1 >> 56 & 15]) << 56 |
			  U64(B_L[x_][y_][d1 >> 48 & 15]) << 48 |
			  U64(B_L[x_][y_][d1 >> 40 & 15]) << 40 |
			  U64(B_L[x_][y_][d1 >> 32 & 15]) << 32 |
			  U64(B_L[x_][y_][d1 >> 24 & 15]) << 24 |
			  U64(B_L[x_][y_][d1 >> 16 & 15]) << 16 |
			  U64(B_L[x_][y_][d1 >>  8 & 15]) <<  8 |
			  U64(B_L[x_][y_][d1 >>  0 & 15])));
		y[i] ^= x[i];
	}
}

int
ra_ec_test(void)
{
	const int K = 255, N = 8192;
	void *buf1, *buf2;
	int i, j;

	/* initialize */

	if (!(buf1 = malloc((K + 2) * N)) || !(buf2 = malloc((K + 2) * N))) {
		RA_FREE(buf1);
		RA_TRACE("out of memory");
		return -1;
	}

	/* populate data blocks */

	memset(buf1, 0, (K + 2) * N);
	memset(buf2, 0, (K + 2) * N);
	for (i=0; i<(K * N); ++i) {
		*((uint8_t *)buf1 + i) = rand() % 256;
	}
	memcpy(buf1, buf2, K * N);

	/* encode P and Q */

	ra_ec_encode_pq(buf1, K, N);

	/* encode P */

	ra_ec_encode_p(buf2, K, N);
	if (memcmp(RA_EC_P(buf1, K, N), RA_EC_P(buf2, K, N), N)) {
		RA_FREE(buf1);
		RA_FREE(buf2);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* encode Q */

	ra_ec_encode_q(buf2, K, N);
	if (memcmp(RA_EC_Q(buf1, K, N), RA_EC_Q(buf2, K, N), N)) {
		RA_FREE(buf1);
		RA_FREE(buf2);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* one D[*]/P encode */

	for (j=0; j<K; ++j) {
		memset(RA_EC_D(buf2, j, N), rand(), N);
		ra_ec_encode_dp(buf2, K, N, j);
	}
	if (memcmp(buf1, buf2, (K + 2) * N)) {
		RA_FREE(buf1);
		RA_FREE(buf2);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* one D[*]/Q encode */

	for (j=0; j<K; ++j) {
		memset(RA_EC_D(buf2, j, N), rand(), N);
		ra_ec_encode_dq(buf2, K, N, j);
	}
	if (memcmp(buf1, buf2, (K + 2) * N)) {
		RA_FREE(buf1);
		RA_FREE(buf2);
		RA_TRACE("integrity failure detected");
		return -1;
	}

	/* two D[*] encode */

	for (i=0; i<K; i+=7) {
		for (j=i+1; j<K; j+=7) {
			memset(RA_EC_D(buf2, i, N), rand(), N);
			memset(RA_EC_D(buf2, j, N), rand(), N);
			ra_ec_encode_dd(buf2, K, N, i, j);
			if (memcmp(RA_EC_D(buf1, i, N),
				   RA_EC_D(buf2, i, N),
				   N) ||
			    memcmp(RA_EC_D(buf1, j, N),
				   RA_EC_D(buf2, j, N),
				   N)) {
				RA_FREE(buf1);
				RA_FREE(buf2);
				RA_TRACE("integrity failure detected");
				return -1;
			}
		}
	}

	/* sanity check data blocks */

	for (j=0; j<K; ++j) {
		if (memcmp(buf1, buf2, (K + 2) * N)) {
			RA_FREE(buf1);
			RA_FREE(buf2);
			RA_TRACE("integrity failure detected");
			return -1;
		}
	}

	/* done */

	RA_FREE(buf1);
	RA_FREE(buf2);
	return 0;
}

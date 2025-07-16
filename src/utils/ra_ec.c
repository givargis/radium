/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_ec.c
 */

#include "../kernel/ra_kernel.h"
#include "ra_ec.h"

static uint8_t G_H[256][16];
static uint8_t G_L[256][16];
static uint8_t A_H[256][16];
static uint8_t A_L[256][16];
static uint8_t B_H[256][256][16];
static uint8_t B_L[256][256][16];

#define U_(x) /*-*/ ( (uint64_t)(x) /*---------------------------*/ )
#define D_(buf,j,n) ( (uint64_t *)((char *)(buf) + ((j) + 0) * (n)) )
#define P_(buf,k,n) ( (uint64_t *)((char *)(buf) + ((k) + 0) * (n)) )
#define Q_(buf,k,n) ( (uint64_t *)((char *)(buf) + ((k) + 1) * (n)) )

static uint8_t
gf_mul(uint8_t a, uint8_t b)
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
gf_pow(uint8_t a, int b)
{
	uint8_t p;

	p = 1;
	b %= 255;
	if (b < 0) {
		b += 255;
	}
	while (b) {
		if (b & 1) {
			p = gf_mul(p, a);
		}
		a = gf_mul(a, a);
		b >>= 1;
	}
	return p;
}

void
ra__ec_init(void)
{
	uint8_t g, a, b;

	/*
	 * G(j) : {02}^j
	 */

	for (int j=0; j<256; ++j) {
		g = gf_pow(2, j);
		for (int i=0; i<16; ++i) {
			G_H[j][i] = gf_mul(g, (uint8_t)(i << 4));
			G_L[j][i] = gf_mul(g, (uint8_t)(i & 15));
		}
	}

	/*
	 * A'(x,y) : A(y - x) | y > x
	 * A(j)    : {02}^j * ( {02}^j + {01} )^−1
	 */

	for (int j=1; j<256; ++j) {
		g = gf_pow(2, j);
		a = gf_mul(g, gf_pow(g^1, 254));
		for (int i=0; i<16; ++i) {
			A_H[j][i] = gf_mul(a, (uint8_t)(i << 4));
			A_L[j][i] = gf_mul(a, (uint8_t)(i & 15));
		}
	}

	/*
	 * B(x,y) : {02}^−x * ( {02}^(y-x) + {01} )^−1 | y > x
	 */

	for (int x=0; x<256; ++x) {
		for (int y=x+1; y<256; ++y) {
			g = gf_pow(2, y - x);
			b = gf_mul(gf_pow(2, 255 - x), gf_pow(g^1, 254));
			for (int i=0; i<16; ++i) {
				B_H[x][y][i] = gf_mul(b, (uint8_t)(i << 4));
				B_L[x][y][i] = gf_mul(b, (uint8_t)(i & 15));
			}
		}
	}
}

void
ra__ec_encode_pq(void *buf, int k, int n)
{
	uint64_t d, *p, *q;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA__EC_MIN_K <= k) && (RA__EC_MAX_K >= k) );
	assert( (RA__EC_MIN_N <= n) && (RA__EC_MAX_N >= n) );

	p = P_(buf, k, n);
	q = Q_(buf, k, n);
	memset(p, 0, n);
	memset(q, 0, n);
	for (int j=0; j<k; ++j) {
		for (int i=0; i<(n/8); ++i) {
			d = D_(buf, j, n)[i];
			p[i] ^= d;
			q[i] ^= ((U_(G_H[j][d >> 60 & 15]) << 56 |
				  U_(G_H[j][d >> 52 & 15]) << 48 |
				  U_(G_H[j][d >> 44 & 15]) << 40 |
				  U_(G_H[j][d >> 36 & 15]) << 32 |
				  U_(G_H[j][d >> 28 & 15]) << 24 |
				  U_(G_H[j][d >> 20 & 15]) << 16 |
				  U_(G_H[j][d >> 12 & 15]) <<  8 |
				  U_(G_H[j][d >>  4 & 15])) ^
				 (U_(G_L[j][d >> 56 & 15]) << 56 |
				  U_(G_L[j][d >> 48 & 15]) << 48 |
				  U_(G_L[j][d >> 40 & 15]) << 40 |
				  U_(G_L[j][d >> 32 & 15]) << 32 |
				  U_(G_L[j][d >> 24 & 15]) << 24 |
				  U_(G_L[j][d >> 16 & 15]) << 16 |
				  U_(G_L[j][d >>  8 & 15]) <<  8 |
				  U_(G_L[j][d >>  0 & 15])));
		}
	}
}

void
ra__ec_encode_p(void *buf, int k, int n)
{
	uint64_t *p;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA__EC_MIN_K <= k) && (RA__EC_MAX_K >= k) );
	assert( (RA__EC_MIN_N <= n) && (RA__EC_MAX_N >= n) );

	p = P_(buf, k, n);
	memset(p, 0, n);
	for (int j=0; j<k; ++j) {
		for (int i=0; i<(n/8); ++i) {
			p[i] ^= D_(buf, j, n)[i];
		}
	}
}

void
ra__ec_encode_q(void *buf, int k, int n)
{
	uint64_t d, *q;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA__EC_MIN_K <= k) && (RA__EC_MAX_K >= k) );
	assert( (RA__EC_MIN_N <= n) && (RA__EC_MAX_N >= n) );

	q = Q_(buf, k, n);
	memset(q, 0, n);
	for (int j=0; j<k; ++j) {
		for (int i=0; i<(n/8); ++i) {
			d = D_(buf, j, n)[i];
			q[i] ^= ((U_(G_H[j][d >> 60 & 15]) << 56 |
				  U_(G_H[j][d >> 52 & 15]) << 48 |
				  U_(G_H[j][d >> 44 & 15]) << 40 |
				  U_(G_H[j][d >> 36 & 15]) << 32 |
				  U_(G_H[j][d >> 28 & 15]) << 24 |
				  U_(G_H[j][d >> 20 & 15]) << 16 |
				  U_(G_H[j][d >> 12 & 15]) <<  8 |
				  U_(G_H[j][d >>  4 & 15])) ^
				 (U_(G_L[j][d >> 56 & 15]) << 56 |
				  U_(G_L[j][d >> 48 & 15]) << 48 |
				  U_(G_L[j][d >> 40 & 15]) << 40 |
				  U_(G_L[j][d >> 32 & 15]) << 32 |
				  U_(G_L[j][d >> 24 & 15]) << 24 |
				  U_(G_L[j][d >> 16 & 15]) << 16 |
				  U_(G_L[j][d >>  8 & 15]) <<  8 |
				  U_(G_L[j][d >>  0 & 15])));
		}
	}
}

void
ra__ec_encode_dp(void *buf, int k, int n, int x_)
{
	uint64_t *d, *p;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA__EC_MIN_K <= k) && (RA__EC_MAX_K >= k) );
	assert( (RA__EC_MIN_N <= n) && (RA__EC_MAX_N >= n) );
	assert( (0 <= x_) && (k > x_) );

	p = P_(buf, k, n);
	d = D_(buf, x_, n);
	memcpy(d, p, n);
	for (int j=0; j<k; ++j) {
		if (j != x_) {
			for (int i=0; i<(n/8); ++i) {
				d[i] ^= D_(buf, j, n)[i];
			}
		}
	}
}

void
ra__ec_encode_dq(void *buf, int k, int n, int x_)
{
	uint64_t d, *q, *x;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (0 < k) && (256 > k) );
	assert( (0 < n) && (0 == (n % 8)) );
	assert( (RA__EC_MIN_K <= k) && (RA__EC_MAX_K >= k) );
	assert( (RA__EC_MIN_N <= n) && (RA__EC_MAX_N >= n) );
	assert( (0 <= x_) && (k > x_) );

	q = Q_(buf, k, n);
	x = D_(buf, x_, n);
	memcpy(x, q, n);
	for (int j=0; j<k; ++j) {
		if (j != x_) {
			for (int i=0; i<(n/8); ++i) {
				d = D_(buf, j, n)[i];
				x[i] ^= ((U_(G_H[j][d >> 60 & 15]) << 56 |
					  U_(G_H[j][d >> 52 & 15]) << 48 |
					  U_(G_H[j][d >> 44 & 15]) << 40 |
					  U_(G_H[j][d >> 36 & 15]) << 32 |
					  U_(G_H[j][d >> 28 & 15]) << 24 |
					  U_(G_H[j][d >> 20 & 15]) << 16 |
					  U_(G_H[j][d >> 12 & 15]) <<  8 |
					  U_(G_H[j][d >>  4 & 15])) ^
					 (U_(G_L[j][d >> 56 & 15]) << 56 |
					  U_(G_L[j][d >> 48 & 15]) << 48 |
					  U_(G_L[j][d >> 40 & 15]) << 40 |
					  U_(G_L[j][d >> 32 & 15]) << 32 |
					  U_(G_L[j][d >> 24 & 15]) << 24 |
					  U_(G_L[j][d >> 16 & 15]) << 16 |
					  U_(G_L[j][d >>  8 & 15]) <<  8 |
					  U_(G_L[j][d >>  0 & 15])));
			}
		}
	}
	x_ = 255 - x_;
	for (int i=0; i<(n/8); ++i) {
		d = x[i];
		x[i] = ((U_(G_H[x_][d >> 60 & 15]) << 56 |
			 U_(G_H[x_][d >> 52 & 15]) << 48 |
			 U_(G_H[x_][d >> 44 & 15]) << 40 |
			 U_(G_H[x_][d >> 36 & 15]) << 32 |
			 U_(G_H[x_][d >> 28 & 15]) << 24 |
			 U_(G_H[x_][d >> 20 & 15]) << 16 |
			 U_(G_H[x_][d >> 12 & 15]) <<  8 |
			 U_(G_H[x_][d >>  4 & 15])) ^
			(U_(G_L[x_][d >> 56 & 15]) << 56 |
			 U_(G_L[x_][d >> 48 & 15]) << 48 |
			 U_(G_L[x_][d >> 40 & 15]) << 40 |
			 U_(G_L[x_][d >> 32 & 15]) << 32 |
			 U_(G_L[x_][d >> 24 & 15]) << 24 |
			 U_(G_L[x_][d >> 16 & 15]) << 16 |
			 U_(G_L[x_][d >>  8 & 15]) <<  8 |
			 U_(G_L[x_][d >>  0 & 15])));
	}
}

void
ra__ec_encode_dd(void *buf, int k, int n, int x_, int y_)
{
	uint64_t d, d1, d2, *p, *q, *x, *y;

	assert( buf );
	assert( 0 == (n % 8) );
	assert( (RA__EC_MIN_K <= k) && (RA__EC_MAX_K >= k) );
	assert( (RA__EC_MIN_N <= n) && (RA__EC_MAX_N >= n) );
	assert( x_ < y_ );
	assert( (0 <= x_) && (k > x_) );
	assert( (0 <= y_) && (k > y_) );

	p = P_(buf, k, n);
	q = Q_(buf, k, n);
	x = D_(buf, x_, n);
	y = D_(buf, y_, n);
	memcpy(x, q, n);
	memcpy(y, p, n);
	for (int j=0; j<k; ++j) {
		if ((j != x_) && (j != y_)) {
			for (int i=0; i<(n/8); ++i) {
				d = D_(buf, j, n)[i];
				y[i] ^= d;
				x[i] ^= ((U_(G_H[j][d >> 60 & 15]) << 56 |
					  U_(G_H[j][d >> 52 & 15]) << 48 |
					  U_(G_H[j][d >> 44 & 15]) << 40 |
					  U_(G_H[j][d >> 36 & 15]) << 32 |
					  U_(G_H[j][d >> 28 & 15]) << 24 |
					  U_(G_H[j][d >> 20 & 15]) << 16 |
					  U_(G_H[j][d >> 12 & 15]) <<  8 |
					  U_(G_H[j][d >>  4 & 15])) ^
					 (U_(G_L[j][d >> 56 & 15]) << 56 |
					  U_(G_L[j][d >> 48 & 15]) << 48 |
					  U_(G_L[j][d >> 40 & 15]) << 40 |
					  U_(G_L[j][d >> 32 & 15]) << 32 |
					  U_(G_L[j][d >> 24 & 15]) << 24 |
					  U_(G_L[j][d >> 16 & 15]) << 16 |
					  U_(G_L[j][d >>  8 & 15]) <<  8 |
					  U_(G_L[j][d >>  0 & 15])));
			}
		}
	}
	for (int i=0; i<(n/8); ++i) {
		d1 = x[i];
		d2 = y[i];
		x[i] = ((U_(A_H[y_ - x_][d2 >> 60 & 15]) << 56 |
			 U_(A_H[y_ - x_][d2 >> 52 & 15]) << 48 |
			 U_(A_H[y_ - x_][d2 >> 44 & 15]) << 40 |
			 U_(A_H[y_ - x_][d2 >> 36 & 15]) << 32 |
			 U_(A_H[y_ - x_][d2 >> 28 & 15]) << 24 |
			 U_(A_H[y_ - x_][d2 >> 20 & 15]) << 16 |
			 U_(A_H[y_ - x_][d2 >> 12 & 15]) <<  8 |
			 U_(A_H[y_ - x_][d2 >>  4 & 15])) ^
			(U_(A_L[y_ - x_][d2 >> 56 & 15]) << 56 |
			 U_(A_L[y_ - x_][d2 >> 48 & 15]) << 48 |
			 U_(A_L[y_ - x_][d2 >> 40 & 15]) << 40 |
			 U_(A_L[y_ - x_][d2 >> 32 & 15]) << 32 |
			 U_(A_L[y_ - x_][d2 >> 24 & 15]) << 24 |
			 U_(A_L[y_ - x_][d2 >> 16 & 15]) << 16 |
			 U_(A_L[y_ - x_][d2 >>  8 & 15]) <<  8 |
			 U_(A_L[y_ - x_][d2 >>  0 & 15]))) ^
			((U_(B_H[x_][y_][d1 >> 60 & 15]) << 56 |
			  U_(B_H[x_][y_][d1 >> 52 & 15]) << 48 |
			  U_(B_H[x_][y_][d1 >> 44 & 15]) << 40 |
			  U_(B_H[x_][y_][d1 >> 36 & 15]) << 32 |
			  U_(B_H[x_][y_][d1 >> 28 & 15]) << 24 |
			  U_(B_H[x_][y_][d1 >> 20 & 15]) << 16 |
			  U_(B_H[x_][y_][d1 >> 12 & 15]) <<  8 |
			  U_(B_H[x_][y_][d1 >>  4 & 15])) ^
			 (U_(B_L[x_][y_][d1 >> 56 & 15]) << 56 |
			  U_(B_L[x_][y_][d1 >> 48 & 15]) << 48 |
			  U_(B_L[x_][y_][d1 >> 40 & 15]) << 40 |
			  U_(B_L[x_][y_][d1 >> 32 & 15]) << 32 |
			  U_(B_L[x_][y_][d1 >> 24 & 15]) << 24 |
			  U_(B_L[x_][y_][d1 >> 16 & 15]) << 16 |
			  U_(B_L[x_][y_][d1 >>  8 & 15]) <<  8 |
			  U_(B_L[x_][y_][d1 >>  0 & 15])));
		y[i] ^= x[i];
	}
}

int
ra__ec_bist(void)
{
	const int K = 255, N = 8192;
	void *buf1, *buf2;
	int e;

	// initialize

	e = 0;
	buf1 = NULL;
	buf2 = NULL;
	if (!(buf1 = ra__malloc((K + 2) * N)) ||
	    !(buf2 = ra__malloc((K + 2) * N))) {
		e = -1;
		RA__ERROR_TRACE(0);
	}

	// populate data blocks

	if (!e) {
		memset(buf1, 0, (K + 2) * N);
		memset(buf2, 0, (K + 2) * N);
		for (int i=0; i<(K * N); ++i) {
			*((uint8_t *)buf1 + i) = rand() % 256;
		}
		memcpy(buf1, buf2, K * N);
	}

	// encode P and Q

	if (!e) {
		ra__ec_encode_pq(buf1, K, N);
	}

	// encode P

	if (!e) {
		ra__ec_encode_p(buf2, K, N);
		if (memcmp(P_(buf1, K, N), P_(buf2, K, N), N)) {
			e = -1;
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		}
	}

	// encode Q

	if (!e) {
		ra__ec_encode_q(buf2, K, N);
		if (memcmp(Q_(buf1, K, N), Q_(buf2, K, N), N)) {
			e = -1;
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		}
	}

	// one D[*]/P encode

	if (!e) {
		for (int j=0; j<K; ++j) {
			memset(D_(buf2, j, N), rand(), N);
			ra__ec_encode_dp(buf2, K, N, j);
		}
		if (memcmp(buf1, buf2, (K + 2) * N)) {
			e = -1;
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		}
	}

	// one D[*]/Q encode

	if (!e) {
		for (int j=0; j<K; ++j) {
			memset(D_(buf2, j, N), rand(), N);
			ra__ec_encode_dq(buf2, K, N, j);
		}
		if (memcmp(buf1, buf2, (K + 2) * N)) {
			e = -1;
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		}
	}

	// two D[*] encode

	if (!e) {
		for (int i=0; i<K; i+=7) {
			for (int j=i+1; j<K; j+=7) {
				memset(D_(buf2, i, N), rand(), N);
				memset(D_(buf2, j, N), rand(), N);
				ra__ec_encode_dd(buf2, K, N, i, j);
				if (memcmp(D_(buf1, i, N),
					   D_(buf2, i, N),
					   N) ||
				    memcmp(D_(buf1, j, N),
					   D_(buf2, j, N),
					   N)) {
					e = -1;
					RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
					break;
				}
			}
			if (e) {
				break;
			}
		}
	}

	// sanity check data blocks

	if (!e) {
		for (int j=0; j<K; ++j) {
			if (memcmp(buf1, buf2, (K + 2) * N)) {
				e = -1;
				RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
				break;
			}
		}
	}

	// done

	RA__FREE(buf1);
	RA__FREE(buf2);
	return e;
}

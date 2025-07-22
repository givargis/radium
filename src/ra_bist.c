//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_bist.c
//

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ra_kernel.h"
#include "ra_base64.h"
#include "ra_eigen.h"
#include "ra_hash.h"
#include "ra_ann.h"
#include "ra_avl.h"
#include "ra_fft.h"
#include "ra_ec.h"
#include "ra_bist.h"

#define BIST(f,m)					\
	do {						\
		uint64_t t = ra_time();			\
		if (f()) {				\
			t = ra_time() - t;		\
			ra_log("error: %10s %6.1fs",	\
			       (m),			\
			       1e-6*t);			\
		}					\
		else {					\
			t = ra_time() - t;		\
			ra_log("info:  %10s %6.1fs OK",	\
			       (m),			\
			       1e-6*t);			\
		}					\
	} while (0)

static int
ann_bist(void)
{
	return 0;
}

static int
avl_bist(void)
{
	const int N = 123456;
	const void *val_;
	ra_avl_t avl;
	char key[32];
	char val[32];

	// initialize

	if (!(avl = ra_avl_open())) {
		RA_TRACE(NULL);
		return -1;
	}
	if (0 != ra_avl_size(avl)) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}

	// single item

	if (ra_avl_update(avl, "key", "val") ||
	    (1 != ra_avl_size(avl)) ||
	    strcmp("val",
		   ra_avl_lookup(avl, "key") ?
		   ra_avl_lookup(avl, "key") : "") ||
	    ra_avl_update(avl, "key", "lav") ||
	    (1 != ra_avl_size(avl)) ||
	    strcmp("lav",
		   ra_avl_lookup(avl, "key") ?
		   ra_avl_lookup(avl, "key") : "")) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}
	ra_avl_delete(avl, "key");
	if (0 != ra_avl_size(avl)) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}

	// random update

	srand(10);
	for (int i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		if (ra_avl_update(avl, key, val)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
		val_ = ra_avl_lookup(avl, key);
		if (!val_ || strcmp(val_, val)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
	}

	// random lookup

	srand(10);
	for (int i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		val_ = ra_avl_lookup(avl, key);
		if (!val_ || strcmp(val_, val)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
	}

	// random delete

	srand(10);
	for (int i=0; i<(N/2); ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		ra_avl_delete(avl, key);
		if (ra_avl_lookup(avl, key)) {
			ra_avl_close(avl);
			RA_TRACE("software");
			return -1;
		}
	}

	// random lookup

	srand(10);
	for (int i=0; i<N; ++i) {
		ra_sprintf(key, sizeof (key), "%d-%d", rand(), i);
		ra_sprintf(val, sizeof (val), "%d-%d", rand(), i);
		val_ = ra_avl_lookup(avl, key);
		if (i < (N / 2)) {
			if (val_) {
				ra_avl_close(avl);
				RA_TRACE("software");
				return -1;
			}
		}
		else {
			if (!val_ || strcmp(val_, val)) {
				ra_avl_close(avl);
				RA_TRACE("software");
				return -1;
			}
		}
	}

	// empty

	ra_avl_empty(avl);
	if (0 != ra_avl_size(avl)) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}

	// done

	ra_avl_close(avl);
	return 0;
}

static int
base64_bist(void)
{
	const int N = 12345;
	size_t len, len_;
	char *buf, *buf_;
	char *s;

	for (int i=0; i<N; ++i) {
		s = NULL;
		buf = NULL;
		buf_ = NULL;
		len = rand() % N + 1;
		if (!(s = malloc(RA_BASE64_ENCODE_LEN(len) + 1)) ||
		    !(buf = malloc(len)) ||
		    !(buf_ = malloc(len))) {
			free(s);
			free(buf);
			free(buf_);
			RA_TRACE("out of memory");
			return -1;
		}
		for (size_t j=0; j<len; ++j) {
			buf[j] = buf_[j] = (char)(rand() % 256);
		}
		ra_base64_encode(buf, len, s);
		if (ra_base64_decode(buf_, &len_, s)) {
			free(s);
			free(buf);
			free(buf_);
			RA_TRACE(NULL);
			return -1;
		}
		if ((len != len_) || memcmp(buf, buf_, len)) {
			free(s);
			free(buf);
			free(buf_);
			RA_TRACE("software");
			return -1;
		}
		free(s);
		free(buf);
		free(buf_);
	}
	return 0;
}

#define D(buf,j,n) ( (uint64_t *)((char *)(buf) + ((j) + 0) * (n)) )
#define P(buf,k,n) ( (uint64_t *)((char *)(buf) + ((k) + 0) * (n)) )
#define Q(buf,k,n) ( (uint64_t *)((char *)(buf) + ((k) + 1) * (n)) )

static int
ec_bist(void)
{
	const int K = 255, N = 8192;
	void *buf1, *buf2;

	// initialize

	if (!(buf1 = malloc((K + 2) * N)) || !(buf2 = malloc((K + 2) * N))) {
		free(buf1);
		RA_TRACE("out of memory");
		return -1;
	}

	// populate data blocks

	memset(buf1, 0, (K + 2) * N);
	memset(buf2, 0, (K + 2) * N);
	for (int i=0; i<(K * N); ++i) {
		*((uint8_t *)buf1 + i) = rand() % 256;
	}
	memcpy(buf1, buf2, K * N);

	// encode P and Q

	ra_ec_encode_pq(buf1, K, N);

	// encode P

	ra_ec_encode_p(buf2, K, N);
	if (memcmp(P(buf1, K, N), P(buf2, K, N), N)) {
		free(buf1);
		free(buf2);
		RA_TRACE("software");
		return -1;
	}

	// encode Q

	ra_ec_encode_q(buf2, K, N);
	if (memcmp(Q(buf1, K, N), Q(buf2, K, N), N)) {
		free(buf1);
		free(buf2);
		RA_TRACE("software");
		return -1;
	}

	// one D[*]/P encode

	for (int j=0; j<K; ++j) {
		memset(D(buf2, j, N), rand(), N);
		ra_ec_encode_dp(buf2, K, N, j);
	}
	if (memcmp(buf1, buf2, (K + 2) * N)) {
		free(buf1);
		free(buf2);
		RA_TRACE("software");
		return -1;
	}

	// one D[*]/Q encode

	for (int j=0; j<K; ++j) {
		memset(D(buf2, j, N), rand(), N);
		ra_ec_encode_dq(buf2, K, N, j);
	}
	if (memcmp(buf1, buf2, (K + 2) * N)) {
		free(buf1);
		free(buf2);
		RA_TRACE("software");
		return -1;
	}

	// two D[*] encode

	for (int i=0; i<K; i+=7) {
		for (int j=i+1; j<K; j+=7) {
			memset(D(buf2, i, N), rand(), N);
			memset(D(buf2, j, N), rand(), N);
			ra_ec_encode_dd(buf2, K, N, i, j);
			if (memcmp(D(buf1, i, N),
				   D(buf2, i, N),
				   N) ||
			    memcmp(D(buf1, j, N),
				   D(buf2, j, N),
				   N)) {
				free(buf1);
				free(buf2);
				RA_TRACE("software");
				return -1;
			}
		}
	}

	// sanity check data blocks

	for (int j=0; j<K; ++j) {
		if (memcmp(buf1, buf2, (K + 2) * N)) {
			free(buf1);
			free(buf2);
			RA_TRACE("software");
			return -1;
		}
	}

	// done

	free(buf1);
	free(buf2);
	return 0;
}

static int
eigen_bist(void)
{
	const double A[6][7][7] = {
		{
			{ -1, 7, 0, 0, 0, 0, 0 },
			{  7, 4, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 }
		},
		{
			{ 1, 2, 3, 0, 0, 0, 0 },
			{ 2, 4, 5, 0, 0, 0, 0 },
			{ 3, 5, 6, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 }
		},
		{
			{ 0.299, 0.363, 0.007, 0.875, 0, 0, 0 },
			{ 0.363, 0.317, 0.055, 0.108, 0, 0, 0 },
			{ 0.007, 0.055, 0.839, 0.862, 0, 0, 0 },
			{ 0.875, 0.108, 0.862, 0.110, 0, 0, 0 },
			{     0,     0,     0,     0, 0, 0, 0 },
			{     0,     0,     0,     0, 0, 0, 0 },
			{     0,     0,     0,     0, 0, 0, 0 }
		},
		{
			{ 1.0000, 0.5000, 0.2500, 0.1250, 0.0625, 0, 0 },
			{ 0.5000, 1.0000, 0.5000, 0.2500, 0.1250, 0, 0 },
			{ 0.2500, 0.5000, 1.0000, 0.5000, 0.2500, 0, 0 },
			{ 0.1250, 0.2500, 0.5000, 1.0000, 0.5000, 0, 0 },
			{ 0.0625, 0.1250, 0.2500, 0.5000, 1.0000, 0, 0 },
			{      0,      0,      0,      0,      0, 0, 0 },
			{      0,      0,      0,      0,      0, 0, 0 }
		},
		{
			{  1.6,  -2.5,  3.4, -4.3,   5.2, -6.1, 0 },
			{ -2.5,   8.6, -9.7, 10.8, -11.9, 12.0, 0 },
			{  3.4,  -9.7,  1.4, -1.5,   1.6, -1.7, 0 },
			{ -4.3,  10.8, -1.5,  1.9,  -2.0,  2.1, 0 },
			{  5.2, -11.9,  1.6, -2.0,   2.3,  2.4, 0 },
			{ -6.1,  12.0, -1.7,  2.1,   2.4,  2.6, 0 },
			{    0,     0,    0,    0,     0,    0, 0 }
		},
		{
			{ 1,  2,  3,  4,  5,  6,  7 },
			{ 2,  8,  9, 10, 11, 12, 13 },
			{ 3,  9, 14, 15, 16, 17, 18 },
			{ 4, 10, 15, 19, 20, 21, 22 },
			{ 5, 11, 16, 20, 23, 24, 25 },
			{ 6, 12, 17, 21, 24, 26, 27 },
			{ 7, 13, 18, 22, 25, 27, 28 }
		}
	};
	const double ABSOLUTE_TOLERANCE = 1e-8;
	double *a, *av, *values, *vectors;
	int n, k = 6;

	while (k--) {
		n = k + 2;
		if (!(a = malloc(n * n * sizeof (a[0]))) ||
		    !(av = malloc(n * n * sizeof (av[0])))) {
			free(a);
			RA_TRACE("out of memory");
			return -1;
		}
		for (int i=0; i<n; ++i) {
			for (int j=0; j<n; ++j) {
				RA_EIGEN_E(a, i, j, n) = A[k][i][j];
			}
		}
		if (ra_eigen(a, &values, &vectors, n)) {
			free(a);
			free(av);
			RA_TRACE(NULL);
			return -1;
		}
		for (int i=0; i<n; ++i) {
			for (int j=0; j<n; ++j) {
				RA_EIGEN_E(av, i,j,n) = 0.0;
				for (int k=0; k<n; ++k) {
					RA_EIGEN_E(av, i, j, n) +=
						RA_EIGEN_E(a, i, k, n) *
						RA_EIGEN_E(vectors, k, j, n);
				}
			}
		}
		for (int i=0; i<n; ++i) {
			for (int j=0; j<n; ++j) {
				double delta = RA_EIGEN_E(av, j, i, n);
				delta /= RA_EIGEN_E(values, 0, i, n);
				delta -= RA_EIGEN_E(vectors, j, i, n);
				if (ABSOLUTE_TOLERANCE < fabs(delta)) {
					free(a);
					free(av);
					free(values);
					free(vectors);
					RA_TRACE("software");
					return -1;
				}
			}
		}
		free(a);
		free(av);
		free(values);
		free(vectors);
	}
	return 0;
}

static int
fft_bist(void)
{
	struct ra_fft_complex signal[8192 * 2];
	struct ra_fft_complex signal_[8192 * 2];
	const double ABSOLUTE_TOLERANCE = 1e-8;
	const int N = RA_ARRAY_SIZE(signal) / 2;

	for (int i=0; i<N; ++i) {
		signal[i].r = .5 - (rand() / (double)RAND_MAX) * 1.0;
		signal[i].i = 0.0;
		signal_[i] = signal[i];
	}
	ra_fft_forward(signal, N);
	ra_fft_inverse(signal, N);
	for (int i=0; i<N; ++i) {
		if ((ABSOLUTE_TOLERANCE < fabs(signal[i].r - signal_[i].r)) ||
		    (ABSOLUTE_TOLERANCE < fabs(signal[i].i - signal_[i].i))) {
			RA_TRACE("software");
			return -1;
		}
	}
	return 0;
}

static int
hash_bist(void)
{
	const int N = 123456;
	double stats[64];
	char buf[128];

	memset(stats, 0, sizeof (stats));
	for (int i=0; i<N; ++i) {
		for (uint64_t j=0; j<RA_ARRAY_SIZE(buf); ++j) {
			buf[j] = (char)rand();
		}
		size_t len = rand() % (sizeof (buf));
		uint64_t hash = ra_hash(buf, len);
		for (int j=0; j<64; ++j) {
			if (hash & (1LU << j)) {
				stats[j] += 1.0;
			}
		}
	}
	for (int i=0; i<64; ++i) {
		stats[i] /= N;
		if ((0.55 < stats[i]) || (0.45 > stats[i])) {
			RA_TRACE("software");
			return -1;
		}
	}
	return 0;
}

int
ra_bist(void)
{
	BIST(ann_bist, "ann");
	BIST(avl_bist, "avl");
	BIST(base64_bist, "base64");
	BIST(ec_bist, "ec");
	BIST(eigen_bist, "eigen");
	BIST(fft_bist, "fft");
	BIST(hash_bist, "hash");
	return 0;
}

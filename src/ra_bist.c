//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_bist.c
//

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ra_ann.h"
#include "ra_avl.h"
#include "ra_base64.h"
#include "ra_ec.h"
#include "ra_eigen.h"
#include "ra_fft.h"
#include "ra_file.h"
#include "ra_hash.h"
#include "ra_int256.h"
#include "ra_json.h"
#include "ra_kernel.h"
#include "ra_sha3.h"
#include "ra_uint256.h"
#include "ra_bist.h"

#define BIST(f, m)				\
	do {					\
		uint64_t t = ra_time();		\
		ra_log(f()			\
		       ? "error: %8s %6.1fs"	\
		       : "info:  %8s %6.1fs",	\
		       (m),			\
		       1e-6 * (ra_time() - t));	\
	} while (0)

static int
argmax(const double *a, int n)
{
	int m;

	m = 0;
	for (int i=1; i<n; ++i) {
		if (a[m] < a[i]) {
			m = i;
		}
	}
	return m;
}

static int
ann_bist(void)
{
	const int TRAIN_N = 60000;
	const int TEST_N = 10000;
	const int BATCH_SIZE = 8;
	const char *s1, *s2;
	uint8_t *images;
	uint8_t *labels;
	size_t n1, n2;
	double *x, *y;
	ra_ann_t ann;
	int errors;

	// load MNIST data

	if (!(s1 = ra_file_read("../data/images.dat")) ||
	    !(s2 = ra_file_read("../data/labels.dat"))) {
		free((void *)s1);
		RA_TRACE(NULL);
		return -1;
	}
	if (!(images = malloc(RA_BASE64_DECODE_LEN(strlen(s1)))) ||
	    !(labels = malloc(RA_BASE64_DECODE_LEN(strlen(s2))))) {
		free((void *)s1);
		free((void *)s2);
		free(images);
		RA_TRACE("out of memory");
		return -1;
	}
	if (ra_base64_decode(images, &n1, s1) ||
	    ra_base64_decode(labels, &n2, s2)) {
		free((void *)s1);
		free((void *)s2);
		free(images);
		free(labels);
		RA_TRACE(NULL);
		return -1;
	}
	free((void *)s1);
	free((void *)s2);

	// sanity check

	assert( (TRAIN_N + TEST_N) * 28 * 28 == n1 );
	assert( (TRAIN_N + TEST_N) *  1 *  1 == n2 );

	// build model

	if (!(ann = ra_ann_open(28 * 28, 10, 100, 4))) {
		free(images);
		free(labels);
		RA_TRACE(NULL);
		return -1;
	}

	// initialize

	if (!(x = malloc(BATCH_SIZE * 28 * 28 * sizeof (x[0]))) ||
	    !(y = malloc(BATCH_SIZE *  1 * 10 * sizeof (y[0])))) {
		free(x);
		free(images);
		free(labels);
		ra_ann_close(ann);
		RA_TRACE("out of memory");
		return -1;
	}

	// train

	for (int i=0; i<(TRAIN_N/BATCH_SIZE); ++i) {
		for (int j=0; j<BATCH_SIZE; ++j) {
			for (int k=0; k<(28*28); ++k) {
				x[j * (28 * 28) + k] = (*images++) / 255.0;
			}
			for (int k=0; k<10; ++k) {
				y[j * 10 + k] = 0.0;
			}
			y[j * 10 + (*labels++)] = 1.0;
		}
		ra_ann_train(ann, x, y, 0.1, BATCH_SIZE);
	}

	// test

	errors = 0;
	for (int i=0; i<TEST_N; ++i) {
		for (int k=0; k<(28*28); ++k) {
			x[k] = (*images++) / 255.0;
		}
		if (argmax(ra_ann_activate(ann, x), 10) != (int)(*labels++)) {
			++errors;
		}
	}

	// cleanup

	images -= (TRAIN_N + TEST_N) * 28 * 28;
	labels -= (TRAIN_N + TEST_N) *  1 *  1;
	free(x);
	free(y);
	free(images);
	free(labels);
	ra_ann_close(ann);

	// verify

	if ((TEST_N * 0.05) < errors) {
		RA_TRACE("software");
		return -1;
	}
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
	if (0 != ra_avl_items(avl)) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}

	// single item

	if (ra_avl_update(avl, "key", "val") ||
	    (1 != ra_avl_items(avl)) ||
	    strcmp("val",
		   ra_avl_lookup(avl, "key") ?
		   ra_avl_lookup(avl, "key") : "") ||
	    ra_avl_update(avl, "key", "lav") ||
	    (1 != ra_avl_items(avl)) ||
	    strcmp("lav",
		   ra_avl_lookup(avl, "key") ?
		   ra_avl_lookup(avl, "key") : "")) {
		ra_avl_close(avl);
		RA_TRACE("software");
		return -1;
	}
	ra_avl_delete(avl, "key");
	if (0 != ra_avl_items(avl)) {
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
	if (0 != ra_avl_items(avl)) {
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
	if (memcmp(RA_EC_P(buf1, K, N), RA_EC_P(buf2, K, N), N)) {
		free(buf1);
		free(buf2);
		RA_TRACE("software");
		return -1;
	}

	// encode Q

	ra_ec_encode_q(buf2, K, N);
	if (memcmp(RA_EC_Q(buf1, K, N), RA_EC_Q(buf2, K, N), N)) {
		free(buf1);
		free(buf2);
		RA_TRACE("software");
		return -1;
	}

	// one D[*]/P encode

	for (int j=0; j<K; ++j) {
		memset(RA_EC_D(buf2, j, N), rand(), N);
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
		memset(RA_EC_D(buf2, j, N), rand(), N);
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
			memset(RA_EC_D(buf2, i, N), rand(), N);
			memset(RA_EC_D(buf2, j, N), rand(), N);
			ra_ec_encode_dd(buf2, K, N, i, j);
			if (memcmp(RA_EC_D(buf1, i, N),
				   RA_EC_D(buf2, i, N),
				   N) ||
			    memcmp(RA_EC_D(buf1, j, N),
				   RA_EC_D(buf2, j, N),
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
file_bist(void)
{
	const char *PATHNAME = "~~~bist~~~";
	const char *QUOTE = "Time is an illusion. Lunchtime is doubly so.";
	const char *s;

	// empty string

	if (ra_file_write(PATHNAME, "")) {
		RA_TRACE(NULL);
		return -1;
	}
	if (!(s = ra_file_read(PATHNAME))) {
		ra_file_unlink(PATHNAME);
		RA_TRACE(NULL);
		return -1;
	}
	if (strlen(s)) {
		free((void *)s);
		ra_file_unlink(PATHNAME);
		RA_TRACE("software");
		return -1;
	}
	free((void *)s);
	ra_file_unlink(PATHNAME);

	// single-byte string

	if (ra_file_write(PATHNAME, "!")) {
		RA_TRACE(NULL);
		return -1;
	}
	if (!(s = ra_file_read(PATHNAME))) {
		ra_file_unlink(PATHNAME);
		RA_TRACE(NULL);
		return -1;
	}
	if (strcmp(s, "!")) {
		free((void *)s);
		ra_file_unlink(PATHNAME);
		RA_TRACE("software");
		return -1;
	}
	free((void *)s);
	ra_file_unlink(PATHNAME);

	// multi-byte

	if (ra_file_write(PATHNAME, QUOTE)) {
		RA_TRACE(NULL);
		return -1;
	}
	if (!(s = ra_file_read(PATHNAME))) {
		ra_file_unlink(PATHNAME);
		RA_TRACE(NULL);
		return -1;
	}
	if (strcmp(s, QUOTE)) {
		free((void *)s);
		ra_file_unlink(PATHNAME);
		RA_TRACE("software");
		return -1;
	}
	free((void *)s);
	ra_file_unlink(PATHNAME);
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

static int
sha3_bist(void)
{
	const char * const IN[] = {
		"Hello World!",
		"I think, therefore I am.",
		"To be, or not to be, that is the question.",
		"The only thing we have to fear is fear itself.",
		"That's one small step for man, one giant leap for mankind."
	};
	const char * const OUT[] = {
		"d0e47486bbf4c16acac26f8b65359297"
		"3c1362909f90262877089f9c8a4536af",
		"776a241fe325a97dbb4c06706c9a7666"
		"d9ccdf3e3b257160f93732f504a686b0",
		"b0ed6cbffe518fc6487729007afe43ec"
		"2c81f75dafb366430ecf6869ee4061eb",
		"4d412e894b376e84f4c410679725212b"
		"7dec0ee0c94bc6679a006708a4b7f9b8",
		"6fef564538f16204a4b1424abdb2e3d3"
		"d3d3a0f9e1357469f0d3dff36857808f"
	};
	uint8_t out[32];
	char out_[65];

	for (int i=0; i<(int)RA_ARRAY_SIZE(IN); ++i) {
		ra_sha3(IN[i], strlen(IN[i]), out);
		for (int j=0; j<32; ++j) {
			ra_sprintf(out_ + 2 * j,
				   sizeof (out_) - 2 * j,
				   "%02x",
				   (int)out[j]);
		}
		if (strcmp(OUT[i], out_)) {
			RA_TRACE("software");
			return -1;
		}
	}
	return 0;
}

static int
int256_bist(void)
{
	const int N = 123456;
	struct ra_uint256 a;
	struct ra_uint256 b;
	struct ra_uint256 q;
	struct ra_uint256 r;
	char buf[96];

	// basic

	if (ra_int256_init(&a, 0) ||
	    ra_int256_is_neg(&a) ||
	    ra_int256_neg(&b, &a) ||
	    ra_int256_is_neg(&b)) {
		RA_TRACE("software");
		return -1;
	}
	ra_int256_string(&a, buf);
	if (strcmp("0", buf)) {
		RA_TRACE("software");
		return -1;
	}
	ra_int256_string(&b, buf);
	if (strcmp("0", buf)) {
		RA_TRACE("software");
		return -1;
	}

	// basic

	if (ra_int256_init(&a,
			   "0x"
			   "ffffffffffffffff"
			   "ffffffffffffffff"
			   "ffffffffffffffff"
			   "ffffffffffffffff") ||
	    !ra_int256_is_neg(&a) ||
	    ra_int256_neg(&b, &a) ||
	    ra_int256_is_neg(&b)) {
		RA_TRACE("software");
		return -1;
	}
	ra_int256_string(&a, buf);
	if (strcmp("-1", buf)) {
		RA_TRACE("software");
		return -1;
	}
	ra_int256_string(&b, buf);
	if (strcmp("1", buf)) {
		RA_TRACE("software");
		return -1;
	}

	// basic

	if (ra_int256_init(&a, "1") ||
	    ra_int256_is_neg(&a) ||
	    ra_int256_neg(&b, &a) ||
	    !ra_int256_is_neg(&b)) {
		RA_TRACE("software");
		return -1;
	}
	ra_int256_string(&a, buf);
	if (strcmp("1", buf)) {
		RA_TRACE("software");
		return -1;
	}
	ra_int256_string(&b, buf);
	if (strcmp("-1", buf)) {
		RA_TRACE("software");
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
		if (ra_int256_div(&q, &a, &b) ||
		    ra_int256_mod(&r, &a, &b) ||
		    ra_int256_mul(&q, &q, &b) ||
		    ra_int256_add(&b, &q, &r) ||
		    ra_int256_cmp(&a, &b)) {
			RA_TRACE("software");
			return -1;
		}
	}
	return 0;
}

static int
json_bist(void)
{
	const struct ra_json_node *node;
	ra_json_t json;

	// empty array

	if (!(json = ra_json_open("[]"))) {
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    node->u.array.node ||
	    (node = node->u.array.link)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	ra_json_close(json);

	// empty object

	if (!(json = ra_json_open("{}"))) {
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    node->u.object.key ||
	    node->u.object.node ||
	    (node = node->u.object.link)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	ra_json_close(json);

	// single-element array

	if (!(json = ra_json_open("[true]"))) {
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA_JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (1 != node->u.array.node->u.bool) ||
	    (node = node->u.array.link)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	ra_json_close(json);

	// single-element object

	if (!(json = ra_json_open("{\"key\":false}"))) {
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.key ||
	    strcmp("key", node->u.object.key) ||
	    !node->u.object.node ||
	    (RA_JSON_NODE_OP_BOOL != node->u.object.node->op) ||
	    (0 != node->u.object.node->u.bool) ||
	    (node = node->u.object.link)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	ra_json_close(json);

	// two-element array

	if (!(json = ra_json_open("[\"hello\",\"world\"]"))) {
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA_JSON_NODE_OP_STRING != node->u.array.node->op) ||
	    !node->u.array.node->u.string ||
	    strcmp("hello", node->u.array.node->u.string) ||
	    !(node = node->u.array.link) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !node->u.array.node ||
	    (RA_JSON_NODE_OP_STRING != node->u.array.node->op) ||
	    !node->u.array.node->u.string ||
	    strcmp("world", node->u.array.node->u.string) ||
	    (node = node->u.array.link)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	ra_json_close(json);

	// two-element object

	if (!(json = ra_json_open("{\"key1\":0.5,\"key2\":-.75}"))) {
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.node ||
	    !node->u.object.key ||
	    strcmp("key1", node->u.object.key) ||
	    (RA_JSON_NODE_OP_NUMBER != node->u.object.node->op) ||
	    (0.5 != node->u.object.node->u.number) ||
	    !(node = node->u.object.link) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.node ||
	    !node->u.object.key ||
	    strcmp("key2", node->u.object.key) ||
	    (RA_JSON_NODE_OP_NUMBER != node->u.object.node->op) ||
	    (-0.75 != node->u.object.node->u.number) ||
	    (node = node->u.object.link)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	ra_json_close(json);

	// nested

	if (!(json = ra_json_open("[[true,false],{\"key\":\"val\"}]"))) {
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.node) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    (RA_JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (1 != node->u.array.node->u.bool) ||
	    !(node = node->u.array.link) ||
	    (RA_JSON_NODE_OP_BOOL != node->u.array.node->op) ||
	    (0 != node->u.array.node->u.bool)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	if (!(node = ra_json_root(json)) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.link) ||
	    (RA_JSON_NODE_OP_ARRAY != node->op) ||
	    !(node = node->u.array.node) ||
	    (RA_JSON_NODE_OP_OBJECT != node->op) ||
	    !node->u.object.key ||
	    strcmp("key", node->u.object.key) ||
	    !(node = node->u.object.node) ||
	    (RA_JSON_NODE_OP_STRING != node->op) ||
	    strcmp("val", node->u.string)) {
		ra_json_close(json);
		RA_TRACE("software");
		return -1;
	}
	ra_json_close(json);
	return 0;
}

static int
uint256_bist(void)
{
	const int N = 123456;
	struct ra_uint256 a;
	struct ra_uint256 b;
	struct ra_uint256 q;
	struct ra_uint256 r;
	char buf[96];
	int set[256];
	int k;

	// basic

	if (ra_uint256_init(&a, 0) ||
	    !ra_uint256_is_zero(&a) ||
	    ra_uint256_init(&a, "") ||
	    !ra_uint256_is_zero(&a) ||
	    ra_uint256_init(&a, "0") ||
	    !ra_uint256_is_zero(&a) ||
	    ra_uint256_init(&a, "0x0") ||
	    !ra_uint256_is_zero(&a) ||
	    ra_uint256_init(&a,
			    "11579208923731619542357098"
			    "50086879078532699846656405"
			    "64039457584007913129639935") ||
	    ra_uint256_init(&b,
			    "0x"
			    "ffffffffffffffff"
			    "ffffffffffffffff"
			    "ffffffffffffffff"
			    "ffffffffffffffff") ||
	    ra_uint256_cmp(&a, &b)) {
		RA_TRACE("software");
		return -1;
	}

	// basic

	for (int i=0; i<N; ++i) {
		if (0 == i) {
			memset(&a, 0x00, sizeof (struct ra_uint256));
		}
		else if (1 == i) {
			memset(&a, 0xff, sizeof (struct ra_uint256));
		}
		else {
			a.hh = ((uint64_t)rand() << 32) + rand();
			a.hl = ((uint64_t)rand() << 32) + rand();
			a.lh = ((uint64_t)rand() << 32) + rand();
			a.ll = ((uint64_t)rand() << 32) + rand();
		}
		ra_uint256_string(&a, buf);
		if (ra_uint256_init(&b, buf) || ra_uint256_cmp(&a, &b)) {
			RA_TRACE("software");
			return -1;
		}
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
		if (ra_uint256_div(&q, &a, &b) ||
		    ra_uint256_mod(&r, &a, &b) ||
		    ra_uint256_mul(&q, &q, &b) ||
		    ra_uint256_add(&b, &q, &r) ||
		    ra_uint256_cmp(&a, &b)) {
			RA_TRACE("software");
			return -1;
		}
	}

	// shl

	for (int i=0; i<N; ++i) {
		RA_UINT256_SET(&a, 0);
		for (int j=0; j<256; ++j) {
			set[j] = rand() % 2;
			switch (set[j]) {
			case 1: ra_uint256_set_bit(&a, j); break;
			case 0: ra_uint256_clr_bit(&a, j); break;
			}
		}
		k = rand() % 256;
		ra_uint256_shl_(&a, &a, k);
		for (int j=0; j<256; ++j) {
			if (j < k) {
				if (ra_uint256_get_bit(&a, j)) {
					RA_TRACE("software");
					return -1;
				}
			}
			else if (set[j - k] && !ra_uint256_get_bit(&a, j)) {
				RA_TRACE("software");
				return -1;
			}
			else if (!set[j - k] && ra_uint256_get_bit(&a, j)) {
				RA_TRACE("software");
				return -1;
			}
		}
	}

	// shr

	for (int i=0; i<N; ++i) {
		RA_UINT256_SET(&a, 0);
		for (int j=0; j<256; ++j) {
			set[j] = rand() % 2;
			switch (set[j]) {
			case 1: ra_uint256_set_bit(&a, 255 - j); break;
			case 0: ra_uint256_clr_bit(&a, 255 - j); break;
			}
		}
		k = rand() % 256;
		ra_uint256_shr_(&a, &a, k);
		for (int j=0; j<256; ++j) {
			if (j < k) {
				if (ra_uint256_get_bit(&a, 255 - j)) {
					RA_TRACE("software");
					return -1;
				}
			}
			else if (set[j - k] &&
				 !ra_uint256_get_bit(&a, 255 - j)) {
				RA_TRACE("software");
				return -1;
			}
			else if (!set[j - k] &&
				 ra_uint256_get_bit(&a, 255 - j)) {
				RA_TRACE("software");
				return -1;
			}
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
	BIST(file_bist, "file");
	BIST(hash_bist, "hash");
	BIST(int256_bist, "int256");
	BIST(json_bist, "json");
	BIST(sha3_bist, "sha3");
	BIST(uint256_bist, "uint256");
	return 0;
}

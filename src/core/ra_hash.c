/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_hash.h"

#define ROT64(x, y) ( ((x) << (y)) | ((x) >> (64 - (y))) )

static void
shuffle(uint64_t *x, uint64_t *y, uint64_t z)
{
	(*x) *= 11400714819323198485UL;
	(*x) = ROT64(*x, 14) ^ z; (*y) += (*x);
	(*x) = ROT64(*x, 25) ^ z; (*y) += (*x);
	(*x) = ROT64(*x, 21) ^ z; (*y) += (*x);
	(*x) = ROT64(*x, 61) ^ z; (*y) += (*x);
	(*x) = ROT64(*x, 34) ^ z; (*y) += (*x);
}

uint64_t
ra_hash(const void *buf_, size_t len)
{
	const char *buf = (const char *)buf_;
	uint64_t x, y, z;
	size_t i, q, r;

	assert( !len || buf );

	/* initialize */

	q = len / 8;
	r = len % 8;
	x = 865713035574157;
	y = 593570735219531;

	/* q's */

	for (i=0; i<q; ++i) {
		memcpy(&z, buf, 8);
		shuffle(&x, &y, z);
		buf += 8;
	}

	/* r's */

	if (r) {
		z = 0;
		memcpy(&z, buf, r);
		shuffle(&x, &y, z);
	}

	/* finalize */

	shuffle(&x, &y, (uint64_t)len);
	return x + y;
}

int
ra_hash_test(void)
{
	const int N = 100000;
	double stats[64];
	char buf[128];
	uint64_t hash;
	size_t len;
	int i, j;

	memset(stats, 0, sizeof (stats));
	for (i=0; i<N; ++i) {
		for (j=0; j<(int)RA_ARRAY_SIZE(buf); ++j) {
			buf[j] = (char)rand();
		}
		len = rand() % (sizeof (buf));
		hash = ra_hash(buf, len);
		for (j=0; j<64; ++j) {
			if (hash & ((uint64_t)1 << j)) {
				stats[j] += 1.0;
			}
		}
	}
	for (i=0; i<64; ++i) {
		stats[i] /= N;
		if ((0.55 < stats[i]) || (0.45 > stats[i])) {
			RA_TRACE("integrity failure detected");
			return -1;
		}
	}
	return 0;
}

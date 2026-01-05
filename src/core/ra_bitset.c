/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_bitset.h"

struct ra_bitset {
	uint64_t ii;
	uint64_t size;
	uint64_t *bitmaps[2];
};

static void
set(struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->bitmaps[s][Q] |= ((uint64_t)1 << R);
}

static void
clr(struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->bitmaps[s][Q] &= ~((uint64_t)1 << R);
}

static int
get(const struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	if (i < bitset->size) {
		if ( (bitset->bitmaps[s][Q] & ((uint64_t)1 << R)) ) {
			return 1;
		}
	}
	return 0;
}

ra_bitset_t
ra_bitset_open(uint64_t size)
{
	struct ra_bitset *bitset;

	assert( size );

	if (!(bitset = malloc(sizeof (struct ra_bitset)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(bitset, 0, sizeof (struct ra_bitset));
	bitset->size = size;
	if (!(bitset->bitmaps[0] = malloc(RA_DUP(bitset->size, 64) * 8)) ||
	    !(bitset->bitmaps[1] = malloc(RA_DUP(bitset->size, 64) * 8))) {
		ra_bitset_close(bitset);
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(bitset->bitmaps[0], 0, RA_DUP(bitset->size, 64) * 8);
	memset(bitset->bitmaps[1], 0, RA_DUP(bitset->size, 64) * 8);
	set(bitset, 0, 0); /* 0 is not a valid allocation */
	return bitset;
}

void
ra_bitset_close(ra_bitset_t bitset)
{
	if (bitset) {
		RA_FREE(bitset->bitmaps[0]);
		RA_FREE(bitset->bitmaps[1]);
		memset(bitset, 0, sizeof (struct ra_bitset));
		RA_FREE(bitset);
	}
}

uint64_t
ra_bitset_reserve(ra_bitset_t bitset, uint64_t n)
{
	uint64_t i, j, k;

	assert( bitset && n );

	for (j=0; j<bitset->size; ++j) {
		i = (bitset->ii + j) % bitset->size;
		for (k=0; k<n; ++k) {
			if (((i+k) >= bitset->size) || get(bitset, 0, i + k)) {
				break;
			}
		}
		if (k == n) {
			for (k=0; k<n; ++k) {
				set(bitset, 0, i + k);
				set(bitset, 1, i + k);
			}
			clr(bitset, 1, i);
			bitset->ii = i + n;
			return i;
		}
	}
	return 0; /* 0 is not a valid allocation */
}

uint64_t
ra_bitset_release(ra_bitset_t bitset, uint64_t i)
{
	uint64_t n;

	assert( bitset && i );

	n = 0;
	if (get(bitset, 0, i) && !get(bitset, 1, i)) {
		do {
			clr(bitset, 0, i);
			clr(bitset, 1, i);
			++n;
			++i;
		}
		while (get(bitset, 1, i));
	}
	return n;
}

uint64_t
ra_bitset_validate(ra_bitset_t bitset, uint64_t i)
{
	uint64_t n;

	assert( bitset );

	n = 0;
	if (get(bitset, 0, i) && !get(bitset, 1, i)) {
		do {
			++n;
			++i;
		}
		while (get(bitset, 1, i));
	}
	return n;
}

uint64_t
ra_bitset_utilized(ra_bitset_t bitset)
{
	uint64_t i, n;

	assert( bitset );

	n = 0;
	for (i=0; i<RA_DUP(bitset->size, 64); ++i) {
		n += __builtin_popcountll(bitset->bitmaps[0][i]);
	}
	return n;
}

uint64_t
ra_bitset_size(ra_bitset_t bitset)
{
	assert( bitset );

	return bitset->size;
}

int
ra_bitset_test(void)
{
	const int N = 2000000000;
	const int M = 10000;
	const int K = 10000000;
	ra_bitset_t bitset;
	uint64_t n;
	int i, j;
	struct {
		uint64_t i;
		uint64_t n;
	} *table;

	/* single bit */

	if (!(bitset = ra_bitset_open(1))) {
		RA_TRACE("^");
		return -1;
	}
	if ((1 != ra_bitset_size(bitset)) ||
	    (1 != ra_bitset_utilized(bitset)) ||
	    (0 != ra_bitset_reserve(bitset, 1))) {
		ra_bitset_close(bitset);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_bitset_close(bitset);

	/* 64 bits */

	if (!(bitset = ra_bitset_open(64))) {
		RA_TRACE("^");
		return -1;
	}
	for (i=0; i<63; ++i) {
		if ((64 != ra_bitset_size(bitset)) ||
		    ((i + 1) != (int)ra_bitset_reserve(bitset, 1)) ||
		    ((i + 2) != (int)ra_bitset_utilized(bitset))) {
			ra_bitset_close(bitset);
			RA_TRACE("integrity failure detected");
			return -1;
		}
	}
	if (ra_bitset_reserve(bitset, 1)) {
		ra_bitset_close(bitset);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_bitset_close(bitset);

	/* random operations */

	if (!(table = malloc(M * sizeof (table[0])))) {
		RA_TRACE("out of memory");
		return -1;
	}
	memset(table, 0, M * sizeof (table[0]));
	if (!(bitset = ra_bitset_open(N))) {
		RA_FREE(table);
		RA_TRACE("^");
		return -1;
	}
	for (i=0; i<K; ++i) {
		j = i % M;
		if (table[j].i) {
			if ((table[j].n != ra_bitset_validate(bitset,
							      table[j].i)) ||
			    (table[j].n != ra_bitset_release(bitset,
							     table[j].i))) {
				ra_bitset_close(bitset);
				RA_FREE(table);
				RA_TRACE("integrity failure detected");
				return -1;
			}
			table[j].i = 0;
			table[j].n = 0;
		}
		else {
			table[j].n = 1 + (rand() % 99);
			table[j].i = ra_bitset_reserve(bitset, table[j].n);
			if (!table[j].i) {
				ra_bitset_close(bitset);
				RA_FREE(table);
				RA_TRACE("^");
				return -1;
			}
		}
	}
	n = ra_bitset_utilized(bitset);
	for (i=0; i<M; ++i) {
		if (table[i].n) {
			if (table[i].n != ra_bitset_release(bitset,
							    table[i].i)) {
				ra_bitset_close(bitset);
				RA_FREE(table);
				RA_TRACE("integrity failure detected");
				return -1;
			}
			n -= table[i].n;
		}
		table[i].i = 0;
		table[i].n = 0;
	}
	if ((1 != n) || (1 != ra_bitset_utilized(bitset))) {
		ra_bitset_close(bitset);
		RA_FREE(table);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_bitset_close(bitset);
	RA_FREE(table);
	return 0;
}

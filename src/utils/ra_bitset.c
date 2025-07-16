/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_bitset.c
 */

#include "ra_bitset.h"

#define NW64(n) RA__DUP(n, 64)

struct ra__bitset {
	uint64_t ii;
	uint64_t capacity;
	uint64_t *memory[2];
};

static void
set(struct ra__bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->memory[s][Q] |= (1LU << R);
}

static void
clr(struct ra__bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->memory[s][Q] &= ~(1LU << R);
}

static int
get(const struct ra__bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	if (i < bitset->capacity) {
		if ( (bitset->memory[s][Q] & (1LU << R)) ) {
			return 1;
		}
	}
	return 0;
}

ra__bitset_t
ra__bitset_open(uint64_t capacity)
{
	struct ra__bitset *bitset;

	assert( capacity );

	if (!(bitset = ra__malloc(sizeof (struct ra__bitset)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(bitset, 0, sizeof (struct ra__bitset));
	bitset->capacity = capacity;
	if (!(bitset->memory[0] = ra__malloc(NW64(capacity) * 8)) ||
	    !(bitset->memory[1] = ra__malloc(NW64(capacity) * 8))) {
		ra__bitset_close(bitset);
	}
	memset(bitset->memory[0], 0, NW64(capacity) * 8);
	memset(bitset->memory[1], 0, NW64(capacity) * 8);
	set(bitset, 0, 0);
	return bitset;
}

void
ra__bitset_close(ra__bitset_t bitset)
{
	if (bitset) {
		RA__FREE(bitset->memory[0]);
		RA__FREE(bitset->memory[1]);
		memset(bitset, 0, sizeof (struct ra__bitset));
	}
	RA__FREE(bitset);
}

uint64_t
ra__bitset_reserve(ra__bitset_t bitset, uint64_t n)
{
	uint64_t i, j_, k;

	assert( bitset );
	assert( n );

	k = n;
	i = bitset->ii;
	for (uint64_t j=0; j<bitset->capacity; ++j) {
		j_ = (bitset->ii + j) % bitset->capacity;
		if (get(bitset, 0, j_)) {
			i = j_ + 1;
			k = n;
			continue;
		}
		if (!--k) {
			for (uint64_t j=0; j<n; ++j) {
				set(bitset, 0, i + j);
				set(bitset, 1, i + j);
			}
			clr(bitset, 1, i);
			bitset->ii = i + n - 1;
			return i;
		}
	}
	return 0;
}

uint64_t
ra__bitset_release(ra__bitset_t bitset, uint64_t i)
{
	uint64_t n;

	assert( bitset );
	assert( i );

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
ra__bitset_validate(ra__bitset_t bitset, uint64_t i)
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
ra__bitset_utilized(ra__bitset_t bitset)
{
	uint64_t n;

	assert( bitset );

	n = 0;
	for (uint64_t i=0; i<NW64(bitset->capacity); ++i) {
		n += ra__popcount(bitset->memory[0][i]);
	}
	return n;
}

uint64_t
ra__bitset_capacity(ra__bitset_t bitset)
{
	assert( bitset );

	return bitset->capacity;
}

int
ra__bitset_bist(void)
{
	uint64_t seg_n[1000];
	uint64_t seg_i[1000];
	ra__bitset_t bitset;
	uint64_t utilized;
	uint64_t n, m;

	// capacity 1

	n = 1;
	if (!(bitset = ra__bitset_open(n)) ||
	    (1 != ra__bitset_utilized(bitset)) ||
	    (n != ra__bitset_capacity(bitset)) ||
	    (0 != ra__bitset_reserve(bitset, 1)) ||
	    (1 != ra__bitset_validate(bitset, 0)) ||
	    (0 != ra__bitset_validate(bitset, 1))) {
		ra__bitset_close(bitset);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__bitset_close(bitset);

	// capacity 63

	n = 63;
	if (!(bitset = ra__bitset_open(n)) ||
	    (1 != ra__bitset_utilized(bitset)) ||
	    (n != ra__bitset_capacity(bitset)) ||
	    (1 != ra__bitset_reserve(bitset, 1)) ||
	    (2 != ra__bitset_reserve(bitset, 2)) ||
	    (4 != ra__bitset_reserve(bitset, n - 4)) ||
	    (1 != ra__bitset_validate(bitset, 0)) ||
	    (1 != ra__bitset_validate(bitset, 1)) ||
	    (2 != ra__bitset_validate(bitset, 2)) ||
	    ((n - 4) != ra__bitset_validate(bitset, 4)) ||
	    (2 != ra__bitset_release(bitset, 2)) ||
	    (2 != ra__bitset_reserve(bitset, 1)) ||
	    (3 != ra__bitset_reserve(bitset, 1)) ||
	    (1 != ra__bitset_validate(bitset, 0)) ||
	    (1 != ra__bitset_validate(bitset, 1)) ||
	    (1 != ra__bitset_validate(bitset, 2)) ||
	    (1 != ra__bitset_validate(bitset, 3)) ||
	    ((n - 4) != ra__bitset_validate(bitset, 4))) {
		ra__bitset_close(bitset);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	ra__bitset_close(bitset);

	// big test

	n = 12345678;
	memset(seg_n, 0, sizeof (seg_n));
	memset(seg_i, 0, sizeof (seg_i));
	if (!(bitset = ra__bitset_open(n)) ||
	    (1 != ra__bitset_utilized(bitset)) ||
	    (n != ra__bitset_capacity(bitset))) {
		ra__bitset_close(bitset);
		RA__ERROR_TRACE(0);
		return -1;
	}
	utilized = 1;
	m = RA__ARRAY_SIZE(seg_n);
	for (uint64_t i=0; i<m; ++i) {
		seg_n[i] = (uint64_t)rand() % (n / m);
		seg_i[i] = ra__bitset_reserve(bitset, seg_n[i]);
		utilized += seg_n[i];
		if (!seg_i[i] ||
		    (utilized != ra__bitset_utilized(bitset)) ||
		    (seg_n[i] != ra__bitset_validate(bitset, seg_i[i]))) {
			ra__bitset_close(bitset);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
	}
	for (uint64_t i=0; i<m; i+=2) {
		utilized -= seg_n[i];
		if ((seg_n[i] != ra__bitset_release(bitset, seg_i[i])) ||
		    (utilized != ra__bitset_utilized(bitset)) ||
		    (0 != ra__bitset_validate(bitset, seg_i[i]))) {
			ra__bitset_close(bitset);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
		seg_n[i] = (uint64_t)rand() % (n / m);
		seg_i[i] = ra__bitset_reserve(bitset, seg_n[i]);
		utilized += seg_n[i];
		if (!seg_i[i] ||
		    (utilized != ra__bitset_utilized(bitset)) ||
		    (seg_n[i] != ra__bitset_validate(bitset, seg_i[i]))) {
			ra__bitset_close(bitset);
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
	}
	ra__bitset_close(bitset);
	return 0;
}

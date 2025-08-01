//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_bitset.c
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "ra_kernel.h"
#include "ra_printf.h"
#include "ra_bitset.h"

struct ra_bitset {
	uint64_t ii;
	uint64_t capacity;
	uint64_t *memory[2];
};

static void
set(struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->memory[s][Q] |= (1LU << R);
}

static void
clr(struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
	const uint64_t Q = i / 64;
	const uint64_t R = i % 64;

	bitset->memory[s][Q] &= ~(1LU << R);
}

static int
get(const struct ra_bitset *bitset, uint64_t s, uint64_t i)
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

ra_bitset_t
ra_bitset_open(uint64_t capacity)
{
	struct ra_bitset *bitset;

	assert( capacity );

	if (!(bitset = malloc(sizeof (struct ra_bitset)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(bitset, 0, sizeof (struct ra_bitset));
	bitset->capacity = capacity;
	if (!(bitset->memory[0] = malloc(RA_DUP(capacity, 64) * 8)) ||
	    !(bitset->memory[1] = malloc(RA_DUP(capacity, 64) * 8))) {
		ra_bitset_close(bitset);
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(bitset->memory[0], 0, RA_DUP(capacity, 64) * 8);
	memset(bitset->memory[1], 0, RA_DUP(capacity, 64) * 8);
	set(bitset, 0, 0);
	return bitset;
}

void
ra_bitset_close(ra_bitset_t bitset)
{
	if (bitset) {
		free(bitset->memory[0]);
		free(bitset->memory[1]);
		memset(bitset, 0, sizeof (struct ra_bitset));
	}
	free(bitset);
}

uint64_t
ra_bitset_reserve(ra_bitset_t bitset, uint64_t n)
{
	uint64_t i, j_, k;

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
ra_bitset_release(ra_bitset_t bitset, uint64_t i)
{
	uint64_t n;

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
	uint64_t n;

	n = 0;
	for (uint64_t i=0; i<RA_DUP(bitset->capacity, 64); ++i) {
		n += ra_popcount(bitset->memory[0][i]);
	}
	return n;
}

uint64_t
ra_bitset_capacity(ra_bitset_t bitset)
{
	return bitset->capacity;
}

//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_bitset.c
//

#include "ra_kernel.h"
#include "ra_bitset.h"

struct ra_bitset {
        uint64_t ii;
        uint64_t size;
        uint64_t *parts[2];
};

static void
set(struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
        const uint64_t Q = i / 64;
        const uint64_t R = i % 64;

        bitset->parts[s][Q] |= (1LLU << R);
}

static void
clr(struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
        const uint64_t Q = i / 64;
        const uint64_t R = i % 64;

        bitset->parts[s][Q] &= ~(1LLU << R);
}

static int
get(const struct ra_bitset *bitset, uint64_t s, uint64_t i)
{
        const uint64_t Q = i / 64;
        const uint64_t R = i % 64;

        if (i < bitset->size) {
                if ( (bitset->parts[s][Q] & (1LLU << R)) ) {
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
                RA_TRACE("out of parts");
                return NULL;
        }
        memset(bitset, 0, sizeof (struct ra_bitset));
        bitset->size = size;
        if (!(bitset->parts[0] = malloc(RA_DUP(bitset->size, 64) * 8)) ||
            !(bitset->parts[1] = malloc(RA_DUP(bitset->size, 64) * 8))) {
                ra_bitset_close(bitset);
                RA_TRACE("out of parts");
                return NULL;
        }
        memset(bitset->parts[0], 0, RA_DUP(bitset->size, 64) * 8);
        memset(bitset->parts[1], 0, RA_DUP(bitset->size, 64) * 8);
        set(bitset, 0, 0);
        return bitset;
}

void
ra_bitset_close(ra_bitset_t bitset)
{
        if (bitset) {
                RA_FREE(bitset->parts[0]);
                RA_FREE(bitset->parts[1]);
                memset(bitset, 0, sizeof (struct ra_bitset));
        }
        RA_FREE(bitset);
}

uint64_t
ra_bitset_reserve(ra_bitset_t bitset, uint64_t n)
{
        uint64_t i, j_, k;

        assert( n );

        k = n;
        i = bitset->ii;
        for (uint64_t j=0; j<bitset->size; ++j) {
                j_ = (bitset->ii + j) % bitset->size;
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
        for (uint64_t i=0; i<RA_DUP(bitset->size, 64); ++i) {
                n += ra_popcount(bitset->parts[0][i]);
        }
        return n;
}

uint64_t
ra_bitset_size(ra_bitset_t bitset)
{
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
        struct {
                uint64_t i;
                uint64_t n;
        } *table;

        // single bit

        if (!(bitset = ra_bitset_open(1))) {
                RA_TRACE("^");
                return -1;
        }
        if ((1 != ra_bitset_size(bitset)) ||
            (1 != ra_bitset_utilized(bitset)) ||
            (0 != ra_bitset_reserve(bitset, 1))) {
                ra_bitset_close(bitset);
                RA_TRACE("software bug detected");
                return -1;
        }
        ra_bitset_close(bitset);

        // 64 bits

        if (!(bitset = ra_bitset_open(64))) {
                RA_TRACE("^");
                return -1;
        }
        for (uint64_t i=0; i<63; ++i) {
                if ((64 != ra_bitset_size(bitset)) ||
                    ((i+1) != ra_bitset_reserve(bitset, 1)) ||
                    ((i+2) != ra_bitset_utilized(bitset))) {
                        ra_bitset_close(bitset);
                        RA_TRACE("software bug detected");
                        return -1;
                }
        }
        if (ra_bitset_reserve(bitset, 1)) {
                ra_bitset_close(bitset);
                RA_TRACE("software bug detected");
                return -1;
        }
        ra_bitset_close(bitset);

        // random operations

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
        for (int i=0; i<K; ++i) {
                int j = i % M;
                if (table[j].i) {
                        if ((table[j].n != ra_bitset_validate(bitset,
                                                              table[j].i)) ||
                            (table[j].n != ra_bitset_release(bitset,
                                                             table[j].i))) {
                                ra_bitset_close(bitset);
                                RA_FREE(table);
                                RA_TRACE("software bug detected");
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
        for (int i=0; i<M; ++i) {
                if (table[i].n) {
                        if (table[i].n != ra_bitset_release(bitset,
                                                            table[i].i)) {
                                ra_bitset_close(bitset);
                                RA_FREE(table);
                                RA_TRACE("software bug detected");
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
                RA_TRACE("software bug detected");
                return -1;
        }
        ra_bitset_close(bitset);
        RA_FREE(table);
        return 0;
}

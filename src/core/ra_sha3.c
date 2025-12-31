/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_sha3.h"

#define ROT64(x, y) ( ((x) << (y)) | ((x) >> (64 - (y))) )

static const int ROTC[24] = {
	1, 3 , 6 , 10, 15, 21, 28, 36, 45, 55, 2, 14,
	27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44
};

static const int PILN[24] = {
	10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4,
	15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1
};

static const uint64_t RNDC[24] = {
	0x0000000000000001, 0x0000000000008082,
	0x800000000000808a, 0x8000000080008000,
	0x000000000000808b, 0x0000000080000001,
	0x8000000080008081, 0x8000000000008009,
	0x000000000000008a, 0x0000000000000088,
	0x0000000080008009, 0x000000008000000a,
	0x000000008000808b, 0x800000000000008b,
	0x8000000000008089, 0x8000000000008003,
	0x8000000000008002, 0x8000000000000080,
	0x000000000000800a, 0x800000008000000a,
	0x8000000080008081, 0x8000000000008080,
	0x0000000080000001, 0x8000000080008008
};

static void
keccakf(uint64_t *s)
{
	uint64_t t, bc[5];
	int i, j, r, k;

	for (r=0; r<24; r++) {
		for (i=0; i<5; i++) {
			bc[i] = s[i +  0] ^
				s[i +  5] ^
				s[i + 10] ^
				s[i + 15] ^
				s[i + 20];
		}
		for (i=0; i<5; i++) {
			t = bc[(i + 4) % 5] ^ ROT64(bc[(i + 1) % 5], 1);
			for (j=0; j<25; j+=5) {
				s[j + i] ^= t;
			}
		}
		t = s[1];
		for (i=0; i<24; i++) {
			k = PILN[i];
			bc[0] = s[k];
			s[k] = ROT64(t, ROTC[i]);
			t = bc[0];
		}
		for (j=0; j<25; j+=5) {
			for (i=0; i<5; i++) {
				bc[i] = s[j + i];
			}
			for (i=0; i<5; i++) {
				s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i+2) % 5];
			}
		}
		s[0] ^= RNDC[r];
	}
}

void
ra_sha3(const void *buf_, size_t len, void *out)
{
	const uint8_t *buf = (const uint8_t *)buf_;
	union {
		uint8_t b[200];
		uint64_t w[25];
	} state;
	uint64_t bi, wi, saved;
	size_t i, q, r, t;
	uint32_t t1, t2;

	assert( !len || buf );

	/* initialize */

	q = len / 8;
	r = len % 8;
	bi = wi = saved = 0;
	memset(&state, 0, sizeof (state));

	/* q's */

	for (i=0; i<q; i++) {
		t = (((uint64_t)(buf[0]) << 8 * 0) |
		     ((uint64_t)(buf[1]) << 8 * 1) |
		     ((uint64_t)(buf[2]) << 8 * 2) |
		     ((uint64_t)(buf[3]) << 8 * 3) |
		     ((uint64_t)(buf[4]) << 8 * 4) |
		     ((uint64_t)(buf[5]) << 8 * 5) |
		     ((uint64_t)(buf[6]) << 8 * 6) |
		     ((uint64_t)(buf[7]) << 8 * 7));
		state.w[wi] ^= t;
		if (17 == ++wi) {
			wi = 0;
			keccakf(state.w);
		}
		buf += 8;
	}

	/* r's */

	while (r--) {
		saved |= (uint64_t)(*(buf++)) << ((bi++) * 8);
	}
	state.w[wi] ^= saved;
	state.w[wi] ^= 0x06LU << ((bi) * 8);
	state.w[16] ^= 0x8000000000000000LU;
	keccakf(state.w);

	/* finalize */

	for (i=0; i<25; i++) {
		t1 = (uint32_t)(state.w[i] >>  0);
		t2 = (uint32_t)(state.w[i] >> 32);
		state.b[i * 8 + 0] = (uint8_t)(t1 >>  0);
		state.b[i * 8 + 1] = (uint8_t)(t1 >>  8);
		state.b[i * 8 + 2] = (uint8_t)(t1 >> 16);
		state.b[i * 8 + 3] = (uint8_t)(t1 >> 24);
		state.b[i * 8 + 4] = (uint8_t)(t2 >>  0);
		state.b[i * 8 + 5] = (uint8_t)(t2 >>  8);
		state.b[i * 8 + 6] = (uint8_t)(t2 >> 16);
		state.b[i * 8 + 7] = (uint8_t)(t2 >> 24);
	}
	memcpy(out, state.b, RA_SHA3_LEN);
}

/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_utils.c
 */

#include "ra_utils.h"

#define TEST(f,m)						\
	do {							\
		uint64_t t = ra__time();			\
		if (f()) {					\
			t = ra__time() - t;			\
			ra__term_color(RA__TERM_COLOR_RED);	\
			ra__term_bold();			\
			printf("\t [FAIL] ");			\
			ra__term_reset();			\
			printf("%20s %6.1fs\n", (m), 1e-6*t);   \
		}						\
		else {						\
			t = ra__time() - t;			\
			ra__term_color(RA__TERM_COLOR_GREEN);	\
			ra__term_bold();			\
			printf("\t [PASS] ");			\
			ra__term_reset();			\
			printf("%20s %6.1fs\n", (m), 1e-6*t);   \
		}						\
	} while (0)

void
ra__utils_init(void)
{
	ra__ec_init();
}

int
ra__utils_bist(void)
{
	TEST(ra__ann_bist, "ann");
	TEST(ra__avl_bist, "avl");
	TEST(ra__base64_bist, "base64");
	TEST(ra__bitset_bist, "bitset");
	TEST(ra__ec_bist, "ec");
	TEST(ra__grid_bist, "grid");
	TEST(ra__fft_bist, "fft");
	TEST(ra__hash_bist, "hash");
	TEST(ra__int256_bist, "int256");
	TEST(ra__json_bist, "json");
	TEST(ra__sha3_bist, "sha3");
	TEST(ra__string_bist, "string");
	TEST(ra__uint256_bist, "uint256");
	return 0;
}

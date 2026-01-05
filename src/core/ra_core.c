/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_core.h"

#define TEST(f, m)						\
	do {							\
		uint64_t t = ra_time();				\
		if (f()) {					\
			e = -1;					\
			ra_printf(RA_COLOR_RED,			\
				  "%8s %6.1fs ERROR\n",		\
				  (m),				\
				  1e-6 * (ra_time() - t));      \
		}						\
		else {						\
			ra_printf(RA_COLOR_GREEN,		\
				  "%8s %6.1fs OK\n",		\
				  (m),				\
				  1e-6 * (ra_time() - t));      \
		}						\
	} while (0)

void
ra_core_init(void)
{
	ra_kernel_init();
	ra_ec_init();
}

int
ra_core_test(void)
{
	int e;

	e = 0;
	TEST(ra_base64_test, "base64");
	TEST(ra_bitset_test, "bitset");
	TEST(ra_ec_test, "ec");
	TEST(ra_fft_test, "fft");
	TEST(ra_file_test, "file");
	TEST(ra_hash_test, "hash");
	TEST(ra_jitc_test, "jitc");
	TEST(ra_json_test, "json");
	TEST(ra_map_test, "map");
	TEST(ra_sha3_test, "sha3");
	return e;
}

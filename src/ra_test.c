//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_test.c
//

#include "ra_kernel.h"
#include "ra_logger.h"
#include "ra_ann.h"
#include "ra_avl.h"
#include "ra_base64.h"
#include "ra_bigint.h"
#include "ra_bitset.h"
#include "ra_ec.h"
#include "ra_eigen.h"
#include "ra_fft.h"
#include "ra_file.h"
#include "ra_hash.h"
#include "ra_json.h"
#include "ra_sha3.h"

#define TEST(f, m)						\
	do {							\
		uint64_t t = ra_time();				\
		if (f()) {					\
			ra_logger(RA_COLOR_RED,			\
				  "%8s %6.1fs ERROR\n",		\
				  (m),				\
				  1e-6 * (ra_time() - t));	\
		}						\
		else {						\
			ra_logger(RA_COLOR_GREEN,		\
				  "%8s %6.1fs OK\n",		\
				  (m),				\
				  1e-6 * (ra_time() - t));	\
		}						\
	} while (0)

void
ra_test(void)
{
	TEST(ra_ann_test, "ann");
	TEST(ra_avl_test, "avl");
	TEST(ra_base64_test, "base64");
	TEST(ra_bigint_test, "bigint");
	TEST(ra_bitset_test, "bitset");
	TEST(ra_ec_test, "ec");
	TEST(ra_eigen_test, "eigen");
	TEST(ra_fft_test, "fft");
	TEST(ra_file_test, "file");
	TEST(ra_hash_test, "hash");
	TEST(ra_json_test, "test");
	TEST(ra_sha3_test, "sha3");
}

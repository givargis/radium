/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_utils.c
 */

#include "ra_utils.h"

#define BIST(f, m)				\
	do {					\
		uint64_t t = ra_time();		\
		ra_log(f()			\
		       ? "error: %8s %6.1fs"	\
		       : "info:  %8s %6.1fs",	\
		       (m),			\
		       1e-6 * (ra_time() - t));	\
	} while (0)

void
ra_utils_init(void)
{
	ra_ec_init();
}

int
ra_utils_bist(void)
{
	BIST(ra_ann_bist, "ann");
	BIST(ra_avl_bist, "avl");
	BIST(ra_base64_bist, "base64");
	BIST(ra_bigint_bist, "bigint");
	BIST(ra_ec_bist, "ec");
	BIST(ra_eigen_bist, "eigen");
	BIST(ra_fft_bist, "fft");
	BIST(ra_hash_bist, "hash");
	BIST(ra_json_bist, "json");
	BIST(ra_sha3_bist, "sha3");
	return 0;
}

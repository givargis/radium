//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra.c
//

#include "ra.h"

#define TEST(f, m)                                              \
        do {                                                    \
                uint64_t t = ra_time();                         \
                if (f()) {                                      \
                        e = -1;                                 \
                        ra_printf(RA_COLOR_RED,                 \
                                  "%8s %6.1fs ERROR\n",         \
                                  (m),                          \
                                  1e-6 * (ra_time() - t));      \
                }                                               \
                else {                                          \
                        ra_printf(RA_COLOR_GREEN,               \
                                  "%8s %6.1fs OK\n",            \
                                  (m),                          \
                                  1e-6 * (ra_time() - t));      \
                }                                               \
        } while (0)

void
ra_init(void)
{
        if ((4 > sizeof (int)) ||
            (8 > sizeof (long)) ||
            (8 != sizeof (size_t)) ||
            (8 != sizeof (void *))) {
                RA_TRACE("unsupported architecture (halting)");
                exit(-1);
        }
        ra_printf(RA_COLOR_CYAN, "\t               ___\n");
        ra_printf(RA_COLOR_CYAN, "\t  _______ ____/ (_)_ ____ _\n");
        ra_printf(RA_COLOR_CYAN, "\t / __/ _ `/ _  / / // /  ' \\\n");
        ra_printf(RA_COLOR_CYAN, "\t/_/  \\_,_/\\_,_/_/\\_,_/_/_/_/\n");
        ra_printf(RA_COLOR_CYAN, "\t----------------------------\n");
        ra_printf(RA_COLOR_GRAY, "\tCopyright (c) Tony Givargis, 2024-2025\n");
        ra_printf(RA_COLOR_GRAY, "\tgivargis@uci.edu\n");
        ra_printf(RA_COLOR_GRAY_BOLD, "\tVersion %s\n\n", RA_VERSION);
}

int
ra_test(void)
{
        int e;

        e = 0;
        TEST(ra_avl_test, "avl");
        TEST(ra_base64_test, "base64");
        TEST(ra_bigint_test, "bigint");
        TEST(ra_bitset_test, "bitset");
        TEST(ra_ec_test, "ec");
        TEST(ra_eigen_test, "eigen");
        TEST(ra_fft_test, "fft");
        TEST(ra_file_test, "file");
        TEST(ra_hash_test, "hash");
        TEST(ra_json_test, "json");
        TEST(ra_mlp_test, "mlp");
        TEST(ra_sha3_test, "sha3");
        return e;
}

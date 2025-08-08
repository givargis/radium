//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_main.c
//

#include "libra/ra.h"

static int
stage(void)
{
        return 0;
}

int
main(int argc, char *argv[])
{
        ra_init();
        for (int i=1; i<argc; ++i) {
                if (!strcmp("--version", argv[i])) {
                        ra_printf(RA_COLOR_BLACK, RA_VERSION);
                        return 0;
                }
                if (!strcmp("--test", argv[i])) {
                        return ra_test();
                }
                ra_printf(RA_COLOR_RED, "error: bad argument '%s'\n", argv[i]);
                return -1;
        }
        return stage();
}

//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_main.c
//

#include "libra/ra.h"

int
main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;
        ra_init();
        return ra_test();
}

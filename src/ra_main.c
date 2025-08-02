//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_main.c
//

#include <string.h>

#include "ra_ec.h"
#include "ra_test.h"
#include "ra_logo.h"
#include "ra_logger.h"

static int
stage(void)
{
	return 0;
}

int
main(int argc, char *argv[])
{
	ra_ec_init();
	for (int i=1; i<argc; ++i) {
		if (!strcmp("--version", argv[i])) {
			ra_logger(RA_COLOR_BLACK, "1.0\n");
			return 0;
		}
		if (!strcmp("--test", argv[i])) {
			ra_test();
			return 0;
		}
		ra_logger(RA_COLOR_RED, "error: bad argument '%s'\n", argv[i]);
		return -1;
	}
	ra_logo();
	return stage();
}

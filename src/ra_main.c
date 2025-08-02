//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_main.c
//

#include <string.h>

#include "ra_ec.h"
#include "ra_test.h"
#include "ra_logo.h"

int
main(int argc, char *argv[])
{
	ra_ec_init();
	for (int i=1; i<argc; ++i) {
		if (!strcmp("--test", argv[i])) {
			ra_test();
			return 0;
		}
	}
	ra_logo();
	return 0;
}

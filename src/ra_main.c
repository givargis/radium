//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_main.c
//

#include "ra_ec.h"
#include "ra_test.h"

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	ra_ec_init();
	ra_test();
	return 0;
}

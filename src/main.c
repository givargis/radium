/* Copyright (c) Tony Givargis, 2024-2026 */

#include "core/ra_core.h"

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	ra_trace_enabled = 1;
	ra_core_init();
	ra_core_test();
	return 0;
}

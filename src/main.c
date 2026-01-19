/* Copyright (c) Tony Givargis, 2024-2026 */

#include "core/ra_core.h"
#include "ra_version.h"

static void
greetings(void)
{
	ra_printf(RA_COLOR_BLUE_BOLD, "%s", RA_GRAPHICS);
	ra_printf(RA_COLOR_GRAY, "%s\n", RA_COPYRIGHT);
	ra_printf(RA_COLOR_GRAY, "Version: %s\n\n", RA_VERSION);
}

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	ra_color_enabled = 1;
	ra_trace_enabled = 1;

	greetings();
	ra_core_init();
	ra_core_test();

	return 0;
}

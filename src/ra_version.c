/* Copyright (c) Tony Givargis, 2024-2026 */

#include "core/ra_core.h"
#include "ra_version.h"

const char *
ra_version(void)
{
	static char buf[32];

	ra_sprintf(buf,
		   sizeof (buf),
		   "%d.%d.%d",
		   RA_VERSION_MAJOR,
		   RA_VERSION_MINOR,
		   RA_VERSION_PATCH);
	return buf;
}

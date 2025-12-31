/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_core.h"

void
ra_core_init(int notrace)
{
	ra_kernel_init(notrace);
	ra_ec_init();
}

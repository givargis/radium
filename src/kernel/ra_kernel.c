/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_kernel.c
 */

#include "ra_kernel.h"

void
ra__kernel_init(int notrace, int nocolor)
{
	ra__core_init();
	ra__term_init(nocolor);
	ra__log_init(notrace);
}

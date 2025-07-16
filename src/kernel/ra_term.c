/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_term.c
 */

#include "ra_core.h"
#include "ra_term.h"

static int _nocolor_;

void
ra__term_init(int nocolor)
{
	_nocolor_ = nocolor ? 1 : 0;
}

void
ra__term_color(int color)
{
	assert( (0 <= color) && (7 >= color) );

	if (!_nocolor_) {
		printf("\033[%dm", 30 + color);
	}
}

void
ra__term_bold(void)
{
	if (!_nocolor_) {
		printf("\033[1m");
	}
}

void
ra__term_reset(void)
{
	if (!_nocolor_) {
		printf("\033[?25h");
		printf("\033[0m");
	}
}

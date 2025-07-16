/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_term.h
 */

#ifndef _RA_TERM_H_
#define _RA_TERM_H_

enum {
	RA__TERM_COLOR_BLACK,
	RA__TERM_COLOR_RED,
	RA__TERM_COLOR_GREEN,
	RA__TERM_COLOR_YELLOW,
	RA__TERM_COLOR_BLUE,
	RA__TERM_COLOR_MAGENTA,
	RA__TERM_COLOR_CYAN,
	RA__TERM_COLOR_GRAY
};

void ra__term_init(int nocolor);

void ra__term_color(int color);

void ra__term_bold(void);

void ra__term_reset(void);

#endif // _RA_TERM_H_

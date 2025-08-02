//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_logger.h
//

#ifndef __RA_PRINTF_H__
#define __RA_PRINTF_H__

#define RA_TRACE(s)				\
	do {					\
		ra_logger(RA_COLOR_YELLOW,	\
			  "trace: %s:%d: %s\n",	\
			  __FILE__,		\
			  __LINE__,		\
			  (s) ? (s) : "");	\
	} while(0)

typedef enum {
	RA_COLOR_BLACK,
	RA_COLOR_BLACK_BOLD,
	RA_COLOR_RED,
	RA_COLOR_RED_BOLD,
	RA_COLOR_GREEN,
	RA_COLOR_GREEN_BOLD,
	RA_COLOR_YELLOW,
	RA_COLOR_YELLOW_BOLD,
	RA_COLOR_BLUE,
	RA_COLOR_BLUE_BOLD,
	RA_COLOR_MAGENTA,
	RA_COLOR_MAGENTA_BOLD,
	RA_COLOR_CYAN,
	RA_COLOR_CYAN_BOLD,
	RA_COLOR_GRAY,
	RA_COLOR_GRAY_BOLD
} ra_color_t;

void ra_logger(ra_color_t color, const char *format, ...);

#endif // __RA_PRINTF_H__

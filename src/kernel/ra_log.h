/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_log.h
 */

#ifndef _RA_LOG_H_
#define _RA_LOG_H_

void ra__log_init(int notrace);

void ra__log(const char *format, ...);

#endif // _RA_LOG_H_

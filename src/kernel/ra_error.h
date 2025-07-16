/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_error.h
 */

#ifndef _RA_ERROR_H_
#define _RA_ERROR_H_

#include "ra_core.h"
#include "ra_log.h"

#define RA__ERROR_TRACE(e)			\
	do {					\
		ra__log("trace: %s:%d: %s",	\
			__FILE__,		\
			__LINE__,		\
			ra__error_string((e)));	\
		if ((e)) {			\
			errno = (e);		\
		}				\
	} while(0)

#define RA__ERROR_HALT(e)			\
	do {					\
		RA__ERROR_TRACE((e));		\
		ra__error_halt();		\
	} while (0)

enum {
	RA__ERROR_KERNEL = -100,
	RA__ERROR_MEMORY,
	RA__ERROR_SYNTAX,
	RA__ERROR_SOFTWARE,
	RA__ERROR_ARGUMENT,
	RA__ERROR_CHECKSUM,
	RA__ERROR_ARITHMETIC,
	RA__ERROR_ARCHITECTURE,
	RA__ERROR_FILE_OPEN,
	RA__ERROR_FILE_STAT,
	RA__ERROR_FILE_READ,
	RA__ERROR_FILE_WRITE,
	RA__ERROR_FILE_UNLINK,
	RA__ERROR_NETWORK_READ,
	RA__ERROR_NETWORK_WRITE,
	RA__ERROR_NETWORK_ADDRESS,
	RA__ERROR_NETWORK_CONNECT,
	RA__ERROR_NETWORK_INTERFACE
};

void ra__error_halt(void);

const char *ra__error_string(int e);

#endif // _RA_ERROR_H_

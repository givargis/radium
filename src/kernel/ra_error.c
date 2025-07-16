/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_error.c
 */

#include "ra_error.h"

void
ra__error_halt(void)
{
	ra__log("error: *** HALT ***");
	exit(-1);
}

const char *
ra__error_string(int e)
{
	switch (e) {
	case 0: return "^";
	case RA__ERROR_KERNEL: return "kernel";
	case RA__ERROR_MEMORY: return "memory";
	case RA__ERROR_SYNTAX: return "syntax";
	case RA__ERROR_SOFTWARE: return "software";
	case RA__ERROR_ARGUMENT: return "argument";
	case RA__ERROR_CHECKSUM: return "checksum";
	case RA__ERROR_ARITHMETIC: return "arithmetic";
	case RA__ERROR_ARCHITECTURE: return "architecture";
	case RA__ERROR_FILE_OPEN: return "file open";
	case RA__ERROR_FILE_STAT: return "file stat";
	case RA__ERROR_FILE_READ: return "file read";
	case RA__ERROR_FILE_WRITE: return "file write";
	case RA__ERROR_FILE_UNLINK: return "file unlink";
	case RA__ERROR_NETWORK_READ: return "network read";
	case RA__ERROR_NETWORK_WRITE: return "network write";
	case RA__ERROR_NETWORK_ADDRESS: return "network address";
	case RA__ERROR_NETWORK_CONNECT: return "network connect";
	case RA__ERROR_NETWORK_INTERFACE: return "network interface";
	}
	return "?";
}

//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_file.h
//

#ifndef __RA_FILE_H__
#define __RA_FILE_H__

char *ra_file_read(const char *pathname);

int ra_file_write(const char *pathname, const char *s);

int ra_file_test(void);

#endif // __RA_FILE_H__

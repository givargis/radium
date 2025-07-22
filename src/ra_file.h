//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_file.h
//

#ifndef _RA_FILE_H_
#define _RA_FILE_H_

char *ra_file_read(const char *pathname);

int ra_file_write(const char *pathname, const char *s);

void ra_file_unlink(const char *pathname);

#endif // _RA_FILE_H_

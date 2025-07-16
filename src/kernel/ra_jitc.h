/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_jitc.h
 */

#ifndef _RA_JITC_H_
#define _RA_JITC_H_

typedef struct ra__jitc *ra__jitc_t;

int ra__jitc_compile(const char *input, const char *output);

ra__jitc_t ra__jitc_open(const char *pathname);

void ra__jitc_close(ra__jitc_t jitc);

long ra__jitc_lookup(ra__jitc_t jitc, const char *symbol);

#endif // _RA_JITC_H_

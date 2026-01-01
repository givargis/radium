/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_JITC_H__
#define __RA_JITC_H__

#include "ra_kernel.h"

typedef struct ra_jitc *ra_jitc_t;

int ra_jitc_compile(const char *input, const char *output);

ra_jitc_t ra_jitc_open(const char *pathname);

void ra_jitc_close(ra_jitc_t jitc);

long ra_jitc_lookup(ra_jitc_t jitc, const char *symbol);

#endif /* __RA_JITC_H__ */

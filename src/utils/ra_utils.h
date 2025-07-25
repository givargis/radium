/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_utils.h
 */

#ifndef _RA_UTILS_H_
#define _RA_UTILS_H_

#include "ra_ann.h"
#include "ra_avl.h"
#include "ra_base64.h"
#include "ra_bigint.h"
#include "ra_ec.h"
#include "ra_eigen.h"
#include "ra_fft.h"
#include "ra_hash.h"
#include "ra_json.h"
#include "ra_sha3.h"

void ra_utils_init(void);

int ra_utils_bist(void);

#endif // _RA_UTILS_H_

/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_utils.h
 */

#ifndef _RA_UTILS_H_
#define _RA_UTILS_H_

#include "ra_ann.h"
#include "ra_avl.h"
#include "ra_base64.h"
#include "ra_bitset.h"
#include "ra_ec.h"
#include "ra_fft.h"
#include "ra_grid.h"
#include "ra_hash.h"
#include "ra_int256.h"
#include "ra_json.h"
#include "ra_sha3.h"
#include "ra_string.h"
#include "ra_uint256.h"

void ra__utils_init(void);

int ra__utils_bist(void);

#endif // _RA_UTILS_H_

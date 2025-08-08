//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra.h
//

#ifndef __RA_H__
#define __RA_H__

#include "ra_avl.h"
#include "ra_base64.h"
#include "ra_bigint.h"
#include "ra_bitset.h"
#include "ra_ec.h"
#include "ra_eigen.h"
#include "ra_fft.h"
#include "ra_file.h"
#include "ra_hash.h"
#include "ra_json.h"
#include "ra_kernel.h"
#include "ra_mlp.h"
#include "ra_network.h"
#include "ra_sha3.h"
#include "ra_thread.h"

#define RA_VERSION "1.0"

void ra_init(void);

int ra_test(void);

#endif // __RA_H__

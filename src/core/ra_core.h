/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_CORE_H__
#define __RA_CORE_H__

#include "ra_map.h"
#include "ra_base64.h"
#include "ra_bigint.h"
#include "ra_bitset.h"
#include "ra_device.h"
#include "ra_ec.h"
#include "ra_fft.h"
#include "ra_file.h"
#include "ra_hash.h"
#include "ra_jitc.h"
#include "ra_json.h"
#include "ra_kernel.h"
#include "ra_mlp.h"
#include "ra_network.h"
#include "ra_sha3.h"
#include "ra_thread.h"
#include "ra_vector.h"

void ra_core_init(void);

int ra_core_test(void);

#endif /* __RA_CORE_H__ */

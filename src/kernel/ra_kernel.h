/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_kernel.h
 */

#ifndef _RA_KERNEL_H_
#define _RA_KERNEL_H_

#include "ra_core.h"
#include "ra_error.h"
#include "ra_file.h"
#include "ra_jitc.h"
#include "ra_kernel.h"
#include "ra_log.h"
#include "ra_network.h"
#include "ra_spinlock.h"
#include "ra_term.h"
#include "ra_thread.h"
#include "ra_wait.h"

void ra__kernel_init(int notrace, int nocolor);

#endif // _RA_KERNEL_H_

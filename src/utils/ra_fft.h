/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_fft.h
 */

#ifndef _RA_FFT_H_
#define _RA_FFT_H_

#include "../kernel/ra_kernel.h"

void ra__fft_forward(struct ra__complex *signal, int n);

void ra__fft_inverse(struct ra__complex *signal, int n);

int ra__fft_bist(void);

#endif // _RA_FFT_H_

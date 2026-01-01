/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_FFT_H__
#define __RA_FFT_H__

#include "ra_kernel.h"

struct ra_fft_complex {
	double r;
	double i;
};

void ra_fft_forward(struct ra_fft_complex *signal, int n);

void ra_fft_inverse(struct ra_fft_complex *signal, int n);

#endif /* __RA_FFT_H__ */

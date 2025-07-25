/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_fft.h
 */

#ifndef _RA_FFT_H_
#define _RA_FFT_H_

#include "../root/ra_root.h"

struct ra_fft_complex {
	double r;
	double i;
};

void ra_fft_forward(struct ra_fft_complex *signal, int n);

void ra_fft_inverse(struct ra_fft_complex *signal, int n);

int ra_fft_bist(void);

#endif // _RA_FFT_H_

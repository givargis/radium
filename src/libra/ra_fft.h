//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_fft.h
//

#ifndef __RA_FFT_H__
#define __RA_FFT_H__

struct ra_fft_complex {
        double r;
        double i;
};

void ra_fft_forward(struct ra_fft_complex *signal, int n);

void ra_fft_inverse(struct ra_fft_complex *signal, int n);

int ra_fft_test(void);

#endif // __RA_FFT_H__

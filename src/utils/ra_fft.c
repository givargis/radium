/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_fft.c
 */

#include "ra_fft.h"

static void
fft(struct ra__complex *v, struct ra__complex *t, int n)
{
	struct ra__complex z, w, *vo, *ve;
	int k;

	if ((k = n / 2)) {
		ve = t;
		vo = t + k;
		for (int i=0; i<k; i++) {
			ve[i] = v[2 * i];
			vo[i] = v[2 * i + 1];
		}
		fft(ve, v, k);
		fft(vo, v, k);
		for (int i=0; i<k; i++) {
			w.r = cos(2 * RA__PI * i / (ra__real_t)n);
			w.i = -sin(2 * RA__PI * i / (ra__real_t)n);
			z.r = w.r * vo[i].r - w.i * vo[i].i;
			z.i = w.r * vo[i].i + w.i * vo[i].r;
			v[i].r = ve[i].r + z.r;
			v[i].i = ve[i].i + z.i;
			v[i + k].r = ve[i].r - z.r;
			v[i + k].i = ve[i].i - z.i;
		}
	}
}

static void
ifft(struct ra__complex *v, struct ra__complex *t, int n)
{
	struct ra__complex z, w, *vo, *ve;
	int k;

	if ((k = n / 2)) {
		k = n / 2;
		ve = t;
		vo = t + k;
		for (int i=0; i<k; i++) {
			ve[i] = v[2 * i];
			vo[i] = v[2 * i + 1];
		}
		ifft(ve, v, k);
		ifft(vo, v, k);
		for (int i=0; i<k; i++) {
			w.r = cos(2 * RA__PI * i / (ra__real_t)n);
			w.i = sin(2 * RA__PI * i / (ra__real_t)n);
			z.r = w.r * vo[i].r - w.i * vo[i].i;
			z.i = w.r * vo[i].i + w.i * vo[i].r;
			v[i].r = ve[i].r + z.r;
			v[i].i = ve[i].i + z.i;
			v[i + k].r = ve[i].r - z.r;
			v[i + k].i = ve[i].i - z.i;
		}
	}
}

void
ra__fft_forward(struct ra__complex *signal, int n)
{
	assert( signal );
	assert( n && (0 == (n & (n - 1))) );

	fft(signal, signal + n, n);
}

void
ra__fft_inverse(struct ra__complex *signal, int n)
{
	assert( signal );
	assert( n && (0 == (n & (n - 1))) );

	for (int i=0; i<n; ++i) {
		signal[i].r /= n;
		signal[i].i /= n;
	}
	ifft(signal, signal + n, n);
}

int
ra__fft_bist(void)
{
	struct ra__complex signal[8192 * 2], signal_[8192 * 2];
	int n;

	n = RA__ARRAY_SIZE(signal) / 2;
	for (int i=0; i<n; ++i) {
		signal[i].r = .5 - (rand() / (ra__real_t)RAND_MAX) * 1.0;
		signal[i].i = 0.0;
		signal_[i] = signal[i];
	}
	ra__fft_forward(signal, n);
	ra__fft_inverse(signal, n);
	for (int i=0; i<n; ++i) {
		if ((0.001 < fabs(signal[i].r - signal_[i].r)) ||
		    (0.001 < fabs(signal[i].i - signal_[i].i))) {
			RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
			return -1;
		}
	}
	return 0;
}

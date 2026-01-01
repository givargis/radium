/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_fft.h"

#define PI 3.14159265358979323846264338327950288

static void
fft(struct ra_fft_complex *v, struct ra_fft_complex *t, int n)
{
	struct ra_fft_complex z, w, *vo, *ve;
	int i, k;

	if ((k = n / 2)) {
		ve = t;
		vo = t + k;
		for (i=0; i<k; i++) {
			ve[i] = v[2 * i];
			vo[i] = v[2 * i + 1];
		}
		fft(ve, v, k);
		fft(vo, v, k);
		for (i=0; i<k; i++) {
			w.r = cos(2 * PI * i / (double)n);
			w.i = -sin(2 * PI * i / (double)n);
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
ifft(struct ra_fft_complex *v, struct ra_fft_complex *t, int n)
{
	struct ra_fft_complex z, w, *vo, *ve;
	int i, k;

	if ((k = n / 2)) {
		ve = t;
		vo = t + k;
		for (i=0; i<k; i++) {
			ve[i] = v[2 * i];
			vo[i] = v[2 * i + 1];
		}
		ifft(ve, v, k);
		ifft(vo, v, k);
		for (i=0; i<k; i++) {
			w.r = cos(2 * PI * i / (double)n);
			w.i = sin(2 * PI * i / (double)n);
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
ra_fft_forward(struct ra_fft_complex *signal, int n)
{
	assert( signal );
	assert( n && (0 == (n & (n - 1))) );

	fft(signal, signal + n, n);
}

void
ra_fft_inverse(struct ra_fft_complex *signal, int n)
{
	int i;

	assert( signal );
	assert( n && (0 == (n & (n - 1))) );

	ifft(signal, signal + n, n);
	for (i=0; i<n; ++i) {
		signal[i].r /= n;
		signal[i].i /= n;
	}
}

//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_eigen.c
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ra_printf.h"
#include "ra_eigen.h"

#define E RA_EIGEN_E

static double *
allocate(int rows, int cols)
{
	double *matrix;

	if (!(matrix = malloc(rows * cols * sizeof (matrix[0])))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	return matrix;
}

static double *
identity(int n)
{
	double *matrix;

	if (!(matrix = allocate(n, n))) {
		RA_TRACE("^");
		return NULL;
	}
	memset(matrix, 0, n * n * sizeof (matrix[0]));
	for (int i=0; i<n; ++i) {
		E(matrix, i, i, n) = 1.0;
	}
	return matrix;
}

static double *
clone(const double *matrix_, int n)
{
	double *matrix;

	if (!(matrix = allocate(n, n))) {
		RA_TRACE("^");
		return NULL;
	}
	memcpy(matrix, matrix_, n * n * sizeof (matrix[0]));
	return matrix;
}

static int
symetric(const double *matrix, int n)
{
	const double ABSOLUTE_TOLERANCE = 1e-8;

	for (int i=0; i<n; ++i) {
		for (int j=i+1; j<n; ++j) {
			double delta = E(matrix, i, j, n) - E(matrix, j, i, n);
			if (ABSOLUTE_TOLERANCE < fabs(delta)) {
				return 0;
			}
		}
	}
	return 1;
}

int
ra_eigen(const double *matrix, double **values, double **vectors, int n)
{
	const double EPSILON = 1e-22;
	const double SIGMA = 1e-44;
	const int K = 100;
	double *tmp;

	assert( matrix );
	assert( values );
	assert( vectors );
	assert( n );

	(*values) = NULL;
	(*vectors) = NULL;

	// symetric?

	if (!symetric(matrix, n)) {
		RA_TRACE("invalid eigen computation on non-symmetric matrix");
		return -1;
	}

	// initialize

	if (!((*values) = allocate(1, n)) ||
	    !((*vectors) = identity(n)) ||
	    !(tmp = clone(matrix, n))) {
		free((*values));
		free((*vectors));
		RA_TRACE("^");
		return -1;
	}

	// Givens rotations

	for (int k=0; k<K; ++k) {
		double t, c, s, alpha, max = -1.0, sum = 0.0;

		// find p, q

		int p = 0, q = 0;
		for (int i=0; i<n; i++) {
			for (int j=i+1; j<n; j++) {
				double mag = fabs(E(tmp, i, j, n));
				if (mag > max) {
					max = mag;
					p = i;
					q = j;
				}
				sum += (mag * mag);
			}
		}

		// terminate?

		if (SIGMA > sum) {
			break;
		}

		// calcuate t, c, s

		alpha = (E(tmp, q,q,n) - E(tmp, p,p,n)) / 2.0 / E(tmp, p,q,n);
		t = sqrt(1.0 + alpha * alpha);
		t = ((alpha > EPSILON) ? 1.0 / (alpha + t) :
		     (alpha < EPSILON) ? 1.0 / (alpha - t) : 1.0);
		c = 1.0 / sqrt(1.0 + t * t);
		s = c * t;

		// rotate

		for (int r=0; r<p; r++) {
			E(tmp, p,r,n) = c * E(tmp, r,p,n) - s * E(tmp, r,q,n);
		}
		for (int r=p+1; r<n; r++) {
			if (r == q) {
				continue;
			}
			E(tmp, r,p,n) = c * E(tmp, p,r,n) - s * E(tmp, q,r,n);
		}
		for (int r=0; r<p; r++) {
			E(tmp, q,r,n) = s * E(tmp, r,p,n) + c * E(tmp, r,q,n);
		}
		for (int r=p+1; r<q; r++) {
			E(tmp, q,r,n) = s * E(tmp, p,r,n) + c * E(tmp, r,q,n);
		}
		for (int r=q+1; r<n; r++) {
			E(tmp, r,q,n) = s * E(tmp, p,r,n) + c * E(tmp, q,r,n);
		}
		E(tmp, p, p, n) = E(tmp, p, p, n) - t * E(tmp, p, q, n);
		E(tmp, q, q, n) = E(tmp, q, q, n) + t * E(tmp, p, q, n);
		E(tmp, q, p, n) = 0.0;

		// symmetrize

		for (int i=0; i<n; i++) {
			for (int j=i+1; j<n; j++) {
				E(tmp, i, j, n) = E(tmp, j, i, n);
			}
		}

		// vectors

		for (int i=0; i<n; i++) {
			double xp = E((*vectors), i, p, n);
			double xq = E((*vectors), i, q, n);
			E((*vectors), i, p, n) = c * xp - s * xq;
			E((*vectors), i, q, n) = s * xp + c * xq;
		}
	}

	// values

	for (int i=0; i<n; i++) {
		E((*values), 0, i, n) = E(tmp, i, i, n);
	}

	// done

	free(tmp);
	return 0;
}

int
ra_eigen_test(void)
{
	const double A[6][7][7] = {
		{
			{ -1, 7, 0, 0, 0, 0, 0 },
			{  7, 4, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 },
			{  0, 0, 0, 0, 0, 0, 0 }
		},
		{
			{ 1, 2, 3, 0, 0, 0, 0 },
			{ 2, 4, 5, 0, 0, 0, 0 },
			{ 3, 5, 6, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0, 0 }
		},
		{
			{ 0.299, 0.363, 0.007, 0.875, 0, 0, 0 },
			{ 0.363, 0.317, 0.055, 0.108, 0, 0, 0 },
			{ 0.007, 0.055, 0.839, 0.862, 0, 0, 0 },
			{ 0.875, 0.108, 0.862, 0.110, 0, 0, 0 },
			{     0,     0,     0,     0, 0, 0, 0 },
			{     0,     0,     0,     0, 0, 0, 0 },
			{     0,     0,     0,     0, 0, 0, 0 }
		},
		{
			{ 1.0000, 0.5000, 0.2500, 0.1250, 0.0625, 0, 0 },
			{ 0.5000, 1.0000, 0.5000, 0.2500, 0.1250, 0, 0 },
			{ 0.2500, 0.5000, 1.0000, 0.5000, 0.2500, 0, 0 },
			{ 0.1250, 0.2500, 0.5000, 1.0000, 0.5000, 0, 0 },
			{ 0.0625, 0.1250, 0.2500, 0.5000, 1.0000, 0, 0 },
			{      0,      0,      0,      0,      0, 0, 0 },
			{      0,      0,      0,      0,      0, 0, 0 }
		},
		{
			{  1.6,  -2.5,  3.4, -4.3,   5.2, -6.1, 0 },
			{ -2.5,   8.6, -9.7, 10.8, -11.9, 12.0, 0 },
			{  3.4,  -9.7,  1.4, -1.5,   1.6, -1.7, 0 },
			{ -4.3,  10.8, -1.5,  1.9,  -2.0,  2.1, 0 },
			{  5.2, -11.9,  1.6, -2.0,   2.3,  2.4, 0 },
			{ -6.1,  12.0, -1.7,  2.1,   2.4,  2.6, 0 },
			{    0,     0,    0,    0,     0,    0, 0 }
		},
		{
			{ 1,  2,  3,  4,  5,  6,  7 },
			{ 2,  8,  9, 10, 11, 12, 13 },
			{ 3,  9, 14, 15, 16, 17, 18 },
			{ 4, 10, 15, 19, 20, 21, 22 },
			{ 5, 11, 16, 20, 23, 24, 25 },
			{ 6, 12, 17, 21, 24, 26, 27 },
			{ 7, 13, 18, 22, 25, 27, 28 }
		}
	};
	const double ABSOLUTE_TOLERANCE = 1e-8;
	double *a, *av, *values, *vectors;
	int n, k = 6;

	while (k--) {
		n = k + 2;
		if (!(a = malloc(n * n * sizeof (a[0]))) ||
		    !(av = malloc(n * n * sizeof (av[0])))) {
			free(a);
			RA_TRACE("out of memory");
			return -1;
		}
		for (int i=0; i<n; ++i) {
			for (int j=0; j<n; ++j) {
				RA_EIGEN_E(a, i, j, n) = A[k][i][j];
			}
		}
		if (ra_eigen(a, &values, &vectors, n)) {
			free(a);
			free(av);
			RA_TRACE("^");
			return -1;
		}
		for (int i=0; i<n; ++i) {
			for (int j=0; j<n; ++j) {
				RA_EIGEN_E(av, i,j,n) = 0.0;
				for (int k=0; k<n; ++k) {
					RA_EIGEN_E(av, i, j, n) +=
						RA_EIGEN_E(a, i, k, n) *
						RA_EIGEN_E(vectors, k, j, n);
				}
			}
		}
		for (int i=0; i<n; ++i) {
			for (int j=0; j<n; ++j) {
				double delta = RA_EIGEN_E(av, j, i, n);
				delta /= RA_EIGEN_E(values, 0, i, n);
				delta -= RA_EIGEN_E(vectors, j, i, n);
				if (ABSOLUTE_TOLERANCE < fabs(delta)) {
					free(a);
					free(av);
					free(values);
					free(vectors);
					RA_TRACE("software bug detected");
					return -1;
				}
			}
		}
		free(a);
		free(av);
		free(values);
		free(vectors);
	}
	return 0;
}

//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_eigen.c
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ra_kernel.h"
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
		RA_TRACE(NULL);
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
		RA_TRACE(NULL);
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
		RA_TRACE("arithmetic");
		return -1;
	}

	// initialize

	if (!((*values) = allocate(1, n)) ||
	    !((*vectors) = identity(n)) ||
	    !(tmp = clone(matrix, n))) {
		free((*values));
		free((*vectors));
		RA_TRACE(NULL);
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

//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_eigen.h
//

#ifndef __RA_EIGEN_H__
#define __RA_EIGEN_H__

#include <stdint.h>

#define RA_EIGEN_E(m, i, j, n) ( (m)[(i) * (uint64_t)(n) + (j)] )

int ra_eigen(const double *matrix,
	     double **values,  // output heap 1xn
	     double **vectors, // output heap nxn (flattend)
	     int n);

int ra_eigen_test(void);

#endif // __RA_EIGEN_H__

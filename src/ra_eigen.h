//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_eigen.h
//

#ifndef _RA_EIGEN_H_
#define _RA_EIGEN_H_

#define RA_EIGEN_E(m, i, j, n) ( (m)[(i) * (uint64_t)(n) + (j)] )

int ra_eigen(const double *matrix,
	     double **values,  // output heap 1xn
	     double **vectors, // output heap nxn (flattend)
	     int n);

#endif // _RA_EIGEN_H_

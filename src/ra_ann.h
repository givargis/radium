//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_ann.h
//

#ifndef __RA_ANN_H__
#define __RA_ANN_H__

typedef struct ra_ann *ra_ann_t;

ra_ann_t ra_ann_open(int input, int output, int hidden, int layers);

void ra_ann_close(ra_ann_t ann);

const double *ra_ann_activate(ra_ann_t ann, const double *x);

void ra_ann_train(ra_ann_t ann,
		  const double *x,
		  const double *y,
		  double learning_rate,
		  int batch_size);

int ra_ann_test(void);

#endif // __RA_ANN_H__

//
// Copyright (c) Tony Givargis, 2024-2025
//
// ra_ann.h
//

#ifndef _RA_ANN_H_
#define _RA_ANN_H_

typedef struct ra_ann *ra_ann_t;

ra_ann_t ra_ann_open(int input, int output, int hidden, int layers);

void ra_ann_close(ra_ann_t ann);

const double *ra_ann_activate(ra_ann_t ann, const double *x);

void ra_ann_train(ra_ann_t ann,
		  const double *x,
		  const double *y,
		  double learning_rate,
		  int batch_size);

#endif // _RA_ANN_H_

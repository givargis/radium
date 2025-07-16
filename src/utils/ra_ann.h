/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_ann.h
 */

#ifndef _RA_ANN_H_
#define _RA_ANN_H_

#include "../kernel/ra_kernel.h"

typedef struct ra__ann *ra__ann_t;

ra__ann_t ra__ann_open(int input, int output, int hidden, int layers);

void ra__ann_close(ra__ann_t ann);

const ra__real_t *ra__ann_activate(ra__ann_t ann, const ra__real_t *x);

ra__real_t ra__ann_train(ra__ann_t ann,
			 const ra__real_t *x,
			 const ra__real_t *y,
			 ra__real_t eta,
			 int k);

int ra__ann_bist(void);

#endif // _RA_ANN_H_

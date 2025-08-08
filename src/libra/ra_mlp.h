//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_mlp.h
//

#ifndef __RA_MLP_H__
#define __RA_MLP_H__

typedef struct ra_mlp *ra_mlp_t;

ra_mlp_t ra_mlp_open(int input, int output, int hidden, int layers);

void ra_mlp_close(ra_mlp_t mlp);

const double *ra_mlp_activate(ra_mlp_t mlp, const double *x);

void ra_mlp_train(ra_mlp_t mlp,
                  const double *x,
                  const double *y,
                  double learning_rate,
                  int batch_size);

int ra_mlp_load(ra_mlp_t mlp, const char *pathname);

int ra_mlp_store(ra_mlp_t mlp, const char *pathname);

int ra_mlp_test(void);

#endif // __RA_MLP_H__

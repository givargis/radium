/* Copyright (c) Tony Givargis, 2024-2026 */

#ifndef __RA_CSV_H__
#define __RA_CSV_H__

#include "ra_kernel.h"

typedef struct ra_csv *ra_csv_t;

ra_csv_t ra_csv_open(const char *s);

void ra_csv_close(ra_csv_t csv);

const char *ra_csv_cell(ra_csv_t csv, uint64_t row, uint64_t col);

uint64_t ra_csv_rows(ra_csv_t csv);

uint64_t ra_csv_cols(ra_csv_t csv);

int ra_csv_print(ra_csv_t csv);

int ra_csv_test(void);

#endif /* __RA_CSV_H__ */

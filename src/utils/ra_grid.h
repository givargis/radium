/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_grid.h
 */

#ifndef _RA_GRID_H_
#define _RA_GRID_H_

typedef struct ra__grid *ra__grid_t;

ra__grid_t ra__grid_open(int rows, int cols);

void ra__grid_close(ra__grid_t grid);

int ra__grid_printf(ra__grid_t grid,
		    int row,
		    int col,
		    const char *format, ...);

void ra__grid_display(ra__grid_t grid);

int ra__grid_bist(void);

#endif // _RA_GRID_H_

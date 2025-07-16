/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_grid.c
 */

#include "../kernel/ra_kernel.h"
#include "ra_grid.h"

#define CELL(t,i,j) ( (t)->cells[(i) * (t)->cols + (j)] )

struct ra__grid {
	int rows;
	int cols;
	int *widths;
	const char **cells;
};

ra__grid_t
ra__grid_open(int rows, int cols)
{
	struct ra__grid *grid;
	uint64_t n;

	assert( 0 < rows );
	assert( 0 < cols );

	// initialize

	if (!(grid = ra__malloc(sizeof (struct ra__grid)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(grid, 0, sizeof (struct ra__grid));
	grid->rows = rows;
	grid->cols = cols;

	// initialize

	n = grid->cols * sizeof (grid->widths[0]);
	if (!(grid->widths = ra__malloc(n))) {
		ra__grid_close(grid);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(grid->widths, 0, n);

	// initialize

	n = grid->rows * grid->cols * sizeof (grid->cells[0]);
	if (!(grid->cells = ra__malloc(n))) {
		ra__grid_close(grid);
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(grid->cells, 0, n);
	return grid;
}

void
ra__grid_close(ra__grid_t grid)
{
	if (grid) {
		if (grid->cells) {
			for (int i=0; i<grid->rows; ++i) {
				for (int j=0; j<grid->cols; ++j) {
					RA__FREE(CELL(grid, i, j));
				}
			}
		}
		RA__FREE(grid->cells);
		RA__FREE(grid->widths);
		memset(grid, 0, sizeof (struct ra__grid));
	}
	RA__FREE(grid);
}

int
ra__grid_printf(ra__grid_t grid, int row, int col, const char *format, ...)
{
	va_list ap;
	int n;

	assert( grid );
	assert( (0 <= row) && (row < grid->rows) );
	assert( (0 <= col) && (col < grid->cols) );
	assert( format );

	va_start(ap, format);
	n = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	RA__FREE(CELL(grid, row, col));
	if (!(CELL(grid, row, col) = ra__malloc(n + 1))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	va_start(ap, format);
	if (n != vsnprintf((char *)CELL(grid, row, col), n + 1, format, ap)) {
		va_end(ap);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	va_end(ap);
	return 0;
}

void
ra__grid_display(ra__grid_t grid)
{
	char format[64];
	int n;

	assert( grid );

	for (int j=0; j<grid->cols; ++j) {
		for (int i=0; i<grid->rows; ++i) {
			grid->widths[j] =
				RA__MAX(grid->widths[j],
					(int)ra__strlen(CELL(grid, i, j)));
		}
	}
	ra__term_color(RA__TERM_COLOR_MAGENTA);
	ra__term_bold();
	printf("+");
	for (int j=0; j<grid->cols; ++j) {
		n = grid->widths[j] + 2;
		for (int i=0; i<n; ++i) {
			printf("-");
		}
		printf("+");
	}
	printf("\n|");
	for (int j=0; j<grid->cols; ++j) {
		ra__term_color(RA__TERM_COLOR_CYAN);
		ra__sprintf(format,
			    sizeof (format),
			    " %%%ds ",
			    grid->widths[j]);
		printf(format, CELL(grid, 0, j) ? CELL(grid, 0, j) : "");
		ra__term_color(RA__TERM_COLOR_MAGENTA);
		ra__term_bold();
		printf("|");
	}
	printf("\n+");
	for (int j=0; j<grid->cols; ++j) {
		n = grid->widths[j] + 2;
		for (int i=0; i<n; ++i) {
			printf("-");
		}
		printf("+");
	}
	for (int i=1; i<grid->rows; ++i) {
		printf("\n|");
		for (int j=0; j<grid->cols; ++j) {
			ra__term_reset();
			ra__sprintf(format,
				    sizeof (format),
				    " %%%ds ",
				    grid->widths[j]);
			printf(format,
			       CELL(grid, i, j) ?
			       CELL(grid, i, j) : "");
			ra__term_color(RA__TERM_COLOR_MAGENTA);
			ra__term_bold();
			printf("|");
		}
	}
	printf("\n+");
	for (int j=0; j<grid->cols; ++j) {
		n = grid->widths[j] + 2;
		for (int i=0; i<n; ++i) {
			printf("-");
		}
		printf("+");
	}
	printf("\n");
	ra__term_reset();
}

int
ra__grid_bist(void)
{
	const int ROWS = 123;
	const int COLS = 456;
	ra__grid_t grid;

	if (!(grid = ra__grid_open(ROWS, COLS))) {
		RA__ERROR_TRACE(0);
		return -1;
	}
	for (int i=0; i<ROWS; ++i) {
		for (int j=0; j<COLS; ++j) {
			if (ra__grid_printf(grid,
					    i,
					    j,
					    "%a",
					    rand() / 1000000.0)) {
				RA__ERROR_TRACE(0);
				return -1;
			}
		}
	}
	ra__grid_close(grid);
	return 0;
}

/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_csv.h"

struct ra_csv {
	char *buf;
	uint64_t n_rows;
	uint64_t n_cols;
	const char **cells;
};

static void
populate(struct ra_csv *csv, uint64_t r, uint64_t c, char *s)
{
	if (csv->cells) {
		while ('\n' == (*s)) {
			++s;
		}
		if (('"' == s[0]) && ('"' == s[strlen(s) - 1])) {
			++s;
			s[strlen(s) - 1] = '\0';
		}
		csv->cells[r * csv->n_cols + c] = s;
	}
}

static void
parse(struct ra_csv *csv)
{
	int quote, field;
	uint64_t j, c, r;
	size_t i, n;
	char *p;

	p = csv->buf;
	j = c = r = 0;
	quote = field = 0;
	n = strlen(csv->buf);
	for (i=0; i<n; ++i) {
		if (csv->buf[i] == '"') {
			if (quote && ('"' == csv->buf[i + 1])) {
				++i;
			}
			else {
				quote = !quote;
				field = 1;
			}
		}
		else if (!quote) {
			if (',' == csv->buf[i]) {
				if (csv->cells) {
					csv->buf[i] = '\0';
					populate(csv, r, j, p);
					p = &csv->buf[i + 1];
				}
				++j;
				field = 0;
			}
			else if ('\n' == csv->buf[i]) {
				if (field || j) {
					if (csv->cells) {
						csv->buf[i] = '\0';
						populate(csv, r, j, p);
						p = &csv->buf[i + 1];
					}
					++j;
				}
				++r;
				c = RA_MAX(c, j);
				j = 0;
				field = 0;
			}
			else if ('\r' == csv->buf[i]) {
				/* ignore */
			}
			else {
				field = 1;
			}
		}
		else {
			field = 1;
		}
	}
	if (field || j) {
		if (csv->cells) {
			csv->buf[i] = '\0';
			populate(csv, r, j, p);
		}
		++j;
		++r;
		c = RA_MAX(c, j);
	}
	csv->n_rows = r;
	csv->n_cols = c;
}

ra_csv_t
ra_csv_open(const char *s)
{
	struct ra_csv *csv;
	uint64_t n;

	assert( s );

	if (!(csv = malloc(sizeof (struct ra_csv)))) {
		RA_TRACE("out of memory");
		return NULL;
	}
	memset(csv, 0, sizeof (struct ra_csv));
	if (!(csv->buf = strdup(s))) {
		ra_csv_close(csv);
		RA_TRACE("out of memory");
		return NULL;
	}
	parse(csv);
	if (csv->n_cols && csv->n_rows) {
		n = csv->n_rows * csv->n_cols * sizeof (csv->cells[0]);
		if (!(csv->cells = malloc(n))) {
			ra_csv_close(csv);
			RA_TRACE("out of memory");
			return NULL;
		}
		memset(csv->cells, 0, n);
		parse(csv);
	}
	else {
		csv->ncols = 0;
		csv->nrows = 0;
	}
	return csv;
}

void
ra_csv_close(ra_csv_t csv)
{
	if (csv) {
		RA_FREE(csv->cells);
		RA_FREE(csv->buf);
		memset(csv, 0, sizeof (struct ra_csv));
		RA_FREE(csv);
	}
}

const char *
ra_csv_cell(ra_csv_t csv, uint64_t row, uint64_t col)
{
	assert( csv );
	assert( row < csv->n_rows );
	assert( col < csv->n_cols );

	return csv->cells[row * csv->n_cols + col];
}

uint64_t
ra_csv_rows(ra_csv_t csv)
{
	assert( csv );

	return csv->n_rows;
}

uint64_t
ra_csv_cols(ra_csv_t csv)
{
	assert( csv );

	return csv->n_cols;
}

int
ra_csv_print(ra_csv_t csv)
{
	const int HEADER_COLOR = RA_COLOR_GREEN_BOLD;
	const int BORDER_COLOR = RA_COLOR_CYAN_BOLD;
	const int TEXT_COLOR = RA_COLOR_BLUE;
	uint64_t i, j, n;
	uint64_t *widths;
	char format[64];

	assert( csv );

	if (!ra_csv_cols(csv) || !ra_csv_rows(csv)) {
		return 0;
	}
	n = ra_csv_cols(csv) * sizeof (widths[0]);
	if (!(widths = malloc(n))) {
		RA_TRACE("out of memory");
		return -1;
	}
	memset(widths, 0, n);
	for (j=0; j<ra_csv_cols(csv); ++j) {
		for (i=0; i<ra_csv_rows(csv); ++i) {
			const char *cell = ra_csv_cell(csv, i, j);
			widths[j] = RA_MAX(widths[j], cell ? strlen(cell) : 0);
		}
	}
	ra_printf(BORDER_COLOR, "+");
	for (j=0; j<ra_csv_cols(csv); ++j) {
		n = widths[j] + 2;
		for (i=0; i<n; ++i) {
			ra_printf(BORDER_COLOR, "-");
		}
		ra_printf(BORDER_COLOR, "+");
	}
	ra_printf(BORDER_COLOR, "\n|");
	for (j=0; j<ra_csv_cols(csv); ++j) {
		ra_sprintf(format, sizeof (format), " %%%ds ", (int)widths[j]);
		ra_printf(HEADER_COLOR,
			  format,
			  ra_csv_cell(csv, 0, j) ?
			  ra_csv_cell(csv, 0, j) : "");
		ra_printf(BORDER_COLOR, "|");
	}
	ra_printf(BORDER_COLOR, "\n+");
	for (j=0; j<ra_csv_cols(csv); ++j) {
		n = widths[j] + 2;
		for (i=0; i<n; ++i) {
			ra_printf(BORDER_COLOR, "-");
		}
		ra_printf(BORDER_COLOR, "+");
	}
	for (i=1; i<ra_csv_rows(csv); ++i) {
		ra_printf(BORDER_COLOR, "\n|");
		for (j=0; j<ra_csv_cols(csv); ++j) {
			ra_sprintf(format,
				   sizeof (format),
				   " %%%ds ",
				   (int)widths[j]);
			ra_printf(TEXT_COLOR,
				  format,
				  ra_csv_cell(csv, i, j) ?
				  ra_csv_cell(csv, i, j) : "");
			ra_printf(BORDER_COLOR, "|");
		}
	}
	ra_printf(BORDER_COLOR, "\n+");
	for (j=0; j<ra_csv_cols(csv); ++j) {
		n = widths[j] + 2;
		for (i=0; i<n; ++i) {
			ra_printf(BORDER_COLOR, "-");
		}
		ra_printf(BORDER_COLOR, "+");
	}
	ra_printf(BORDER_COLOR, "\n");
	RA_FREE(widths);
	return 0;
}

int
ra_csv_test(void)
{
	uint64_t i, r, c;
	char buf[3072];
	ra_csv_t csv;

	/* empty */

	ra_sprintf(buf, sizeof (buf), "");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((0 != ra_csv_rows(csv)) || (0 != ra_csv_cols(csv))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* single cell */

	ra_sprintf(buf, sizeof (buf), "abc;def");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((1 != ra_csv_rows(csv)) ||
	    (1 != ra_csv_cols(csv)) ||
	    strcmp("abc;def", ra_csv_cell(csv, 0, 0))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* blank row */

	ra_sprintf(buf, sizeof (buf), "\n\nabc\n");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((3 != ra_csv_rows(csv)) ||
	    (1 != ra_csv_cols(csv)) ||
	    strcmp("abc", ra_csv_cell(csv, 2, 0))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* blank lead col */

	ra_sprintf(buf, sizeof (buf), ",abc\n");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((1 != ra_csv_rows(csv)) ||
	    (2 != ra_csv_cols(csv)) ||
	    strcmp("abc", ra_csv_cell(csv, 0, 1))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* blank tail col */

	ra_sprintf(buf, sizeof (buf), "abc,");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((1 != ra_csv_rows(csv)) ||
	    (2 != ra_csv_cols(csv)) ||
	    strcmp("abc", ra_csv_cell(csv, 0, 0))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* blank mid col */

	ra_sprintf(buf, sizeof (buf), "abc,,xyz\n");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((1 != ra_csv_rows(csv)) ||
	    (3 != ra_csv_cols(csv)) ||
	    strcmp("abc", ra_csv_cell(csv, 0, 0)) ||
	    strcmp(""   , ra_csv_cell(csv, 0, 1)) ||
	    strcmp("xyz", ra_csv_cell(csv, 0, 2))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* quote */

	ra_sprintf(buf, sizeof (buf), "abc,\"x,y\"\n");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((1 != ra_csv_rows(csv)) ||
	    (2 != ra_csv_cols(csv)) ||
	    strcmp("abc", ra_csv_cell(csv, 0, 0)) ||
	    strcmp("x,y", ra_csv_cell(csv, 0, 1))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* quote within quote */

	ra_sprintf(buf, sizeof (buf), "abc,\"x,y\"\n\"\"x\"\"");
	if (!(csv = ra_csv_open(buf))) {
		RA_TRACE("^");
		return -1;
	}
	if ((2 != ra_csv_rows(csv)) ||
	    (2 != ra_csv_cols(csv)) ||
	    strcmp("abc", ra_csv_cell(csv, 0, 0)) ||
	    strcmp("x,y", ra_csv_cell(csv, 0, 1)) ||
	    strcmp("\"x\"", ra_csv_cell(csv, 1, 0))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	ra_csv_close(csv);

	/* random */

	srand(10);
	for (i=r=0; r<100; ++r) {
		c = (uint64_t)rand() % 26;
		while (c) {
			buf[i++] = 'A' + c;
			buf[i++] = ',';
			--c;
		}
		assert( i < sizeof (buf) );
		buf[i++] = '\n';
		buf[i] = '\0';
	}
	if (!(csv = ra_csv_open(buf))) {
		ra_csv_close(csv);
		RA_TRACE("^");
		return -1;
	}
	if ((100 != ra_csv_rows(csv)) || (26 != ra_csv_cols(csv))) {
		ra_csv_close(csv);
		RA_TRACE("integrity failure detected");
		return -1;
	}
	srand(10);
	for (r=0; r<100; ++r) {
		i = 0;
		c = (uint64_t)rand() % 26;
		while (c) {
			if (('A' + c) !=
			    (uint64_t)ra_csv_cell(csv, r, i)[0]) {
				ra_csv_close(csv);
				RA_TRACE("integrity failure detected");
				return -1;
			}
			++i;
			--c;
		}
	}
	ra_csv_close(csv);
	return 0;
}

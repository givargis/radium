/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_string.c
 */

#include "../kernel/ra_kernel.h"
#include "ra_string.h"

struct ra__string {
	int len;
	char *buf;
};

static int
grow(struct ra__string *string, int len)
{
	char *buf;

	if (len > string->len) {
		if (!(buf = ra__realloc(string->buf, len))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		buf[string->len] = '\0';
		string->len = len;
		string->buf = buf;
	}
	return 0;
}

ra__string_t
ra__string_open(void)
{
	struct ra__string *string;

	if (!(string = ra__malloc(sizeof (struct ra__string)))) {
		RA__ERROR_TRACE(0);
		return NULL;
	}
	memset(string, 0, sizeof (struct ra__string));
	return string;
}

void
ra__string_close(ra__string_t string, const char **buf)
{
	if (string) {
		if (buf) {
			(*buf) = string->buf;
			string->buf = NULL;
		}
		RA__FREE(string->buf);
		memset(string, 0, sizeof (struct ra__string));
	}
	RA__FREE(string);
}

void
ra__string_truncate(ra__string_t string)
{
	assert( string );

	RA__FREE(string->buf);
	string->len = 0;
	string->buf = NULL;
}

int
ra__string_append(ra__string_t string, const char *format, ...)
{
	va_list ap;
	int n, len;

	assert( string );
	assert( format );

	va_start(ap, format);
	len = (int)ra__strlen(string->buf);
	if (string->len <= (n = vsnprintf(string->buf + len,
					  string->len - len,
					  format,
					  ap)) + len) {
		va_end(ap);
		if (grow(string, len + (2 * n + 1))) {
			RA__ERROR_TRACE(0);
			return -1;
		}
		va_start(ap, format);
		if (string->len <= vsnprintf(string->buf + len,
					     string->len - len,
					     format,
					     ap)) {
			va_end(ap);
			RA__ERROR_HALT(RA__ERROR_SOFTWARE);
			return -1;
		}
	}
	va_end(ap);
	return 0;
}

void
ra__string_trim(ra__string_t string)
{
	const char *b, *e;
	uint64_t n;
	char *s;

	assert( string );

	s = string->buf;
	b = s;
	e = s + ra__strlen(s) - 1;
	while ((b <= e) && isspace((unsigned char)(*b))) {
		++b;
	}
	while ((e >= s) && isspace((unsigned char)(*e))) {
		--e;
	}
	n = (b <= e) ? (e - b + 1) : 0;
	memmove(s, b, n);
	s[n] = '\0';
}

void
ra__string_unspace(ra__string_t string)
{
	char *s;

	assert( string );

	s = string->buf;
	while (*s) {
		if (isspace((unsigned char)(*s))) {
			memmove(s, s + 1, ra__strlen(s + 1) + 1);
		}
		else {
			++s;
		}
	}
}

const char *
ra__string_buf(ra__string_t string)
{
	assert( string );

	return string->buf;
}

int
ra__string_bist(void)
{
	ra__string_t string;

	// initialize

	if (!(string = ra__string_open())) {
		RA__ERROR_TRACE(0);
		return -1;
	}

	// append

	if (ra__string_append(string, "")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	if ((NULL == ra__string_buf(string)) ||
	    strcmp(ra__string_buf(string), "")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	if (ra__string_append(string, "%d", 12345678)) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	if (strcmp(ra__string_buf(string), "12345678")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	if (ra__string_append(string, "%s", "struct ra__string")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	if (strcmp(ra__string_buf(string), "12345678struct ra__string")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// trim

	ra__string_truncate(string);
	if (ra__string_append(string, "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, " test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "test ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, " test ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, " te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "te st ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, " te st ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "t")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "t")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "  t")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "t")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "t  ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "t")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "  t  ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_trim(string);
	if (strcmp(ra__string_buf(string), "t")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// unspace

	ra__string_truncate(string);
	if (ra__string_append(string, "te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_unspace(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, " te st")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_unspace(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, "te st ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_unspace(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}
	/*-*/
	ra__string_truncate(string);
	if (ra__string_append(string, " te st ")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(0);
		return -1;
	}
	ra__string_unspace(string);
	if (strcmp(ra__string_buf(string), "test")) {
		ra__string_close(string, NULL);
		RA__ERROR_TRACE(RA__ERROR_SOFTWARE);
		return -1;
	}

	// done

	ra__string_close(string, NULL);
	return 0;
}

//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_kernel.c
//

#include <sys/time.h>
#include <unistd.h>

#include "ra_kernel.h"

int
ra_endian(void)
{
        const uint32_t SAMPLE = 0x87654321;
        const uint8_t *probe;

        probe = (const uint8_t *)&SAMPLE;
        if ((0x21 == probe[0])) {
                if ((0x43 != probe[1]) ||
                    (0x65 != probe[2]) ||
                    (0x87 != probe[3])) {
                        RA_TRACE("unsupported architecture (halting)");
                        exit(-1);
                }
                return 0;
        }
        return 1;
}

int
ra_cores(void)
{
        long cores;

        if (0 >= (cores = sysconf(_SC_NPROCESSORS_ONLN))) {
                RA_TRACE("unable to determine kernel core count (halting)");
                exit(-1);
        }
        return (int)cores;
}

size_t
ra_page(void)
{
        long page;

        if (0 >= (page = sysconf(_SC_PAGESIZE))) {
                RA_TRACE("unable to determine kernel page size (halting)");
                exit(-1);
        }
        return (size_t)page;
}

size_t
ra_memory(void)
{
        long page, pages;

        if ((0 >= (page = sysconf(_SC_PAGESIZE))) ||
            (0 >= (pages = sysconf(_SC_PHYS_PAGES)))) {
                RA_TRACE("unable to determine kernel page size (halting)");
                exit(-1);
        }
        return (size_t)page * (size_t)pages;
}

uint64_t
ra_time(void)
{
        struct timeval timeval;

        if (gettimeofday(&timeval, 0)) {
                RA_TRACE("kernel failure detected");
                return 0;
        }
        return (uint64_t)timeval.tv_sec * 1000000 + (uint64_t)timeval.tv_usec;
}

char *
ra_strdup(const char *s)
{
        size_t n;
        char *p;

        n = ra_strlen(s);
        if (!(p = malloc(n + 1))) {
                RA_TRACE("out of memory");
                return NULL;
        }
        memcpy(p, s, n);
        p[n] = '\0';
        return p;
}

void
ra_printf(ra_color_t color, const char *format, ...)
{
        int is_term;
        va_list ap;

        assert( (RA_COLOR_BLACK <= color) && (RA_COLOR_GRAY_BOLD >= color) );
        assert( format );

        if ((is_term = isatty(STDOUT_FILENO))) {
                fprintf(stderr, "\033[%dm", color / 2 + 30);
                if (0 != (color % 2)) {
                        fprintf(stderr, "\033[1m");
                }
        }
        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);
        if (is_term) {
                fprintf(stderr, "\033[0m");
        }
        fflush(stdout);
}

void
ra_sprintf(char *buf, size_t len, const char *format, ...)
{
        va_list ap;

        assert( !len || buf );
        assert( format );

        va_start(ap, format);
        if ((long long)len <= vsnprintf(buf, len, format, ap)) {
                va_end(ap);
                RA_TRACE("software bug detected (halting)");
                exit(-1);
        }
        va_end(ap);
}

const char *
ra_string(const char *format, ...)
{
        va_list ap;
        size_t n;
        char *p;

        assert( format );

        va_start(ap, format);
        n = vsnprintf(NULL, 0, format, ap);
        va_end(ap);
        if (!(p = malloc(n + 1))) {
                RA_TRACE("out of memory");
                return NULL;
        }
        va_start(ap, format);
        n -= vsnprintf(p, n + 1, format, ap);
        va_end(ap);
        if (n) {
                RA_FREE(p);
                RA_TRACE("software bug detected (halting)");
                exit(-1);
        }
        return p;
}

const char *
ra_pathname(const char *name)
{
        const char *path;
        char *pathname;
        size_t n;

        if (!(path = getenv("TMPDIR")) &&
            !(path = getenv("TEMP")) &&
            !(path = getenv("TMP"))) {
                path = "";
        }
        n = ra_strlen(path) + (ra_strlen(name) ? ra_strlen(name) : 32) + 2;
        if (!(pathname = malloc(n))) {
                RA_TRACE("out of memory");
                return NULL;
        }
        if (name && strlen(name)) {
                ra_sprintf(pathname,
                           n,
                           "%s%s%s",
                           path,
                           strlen(path) ? "/" : "",
                           name);
        }
        else {
                ra_sprintf(pathname,
                           n,
                           "%s%s%d.%d",
                           path,
                           strlen(path) ? "/" : "",
                           (int)rand(),
                           (int)ra_time());
        }
        return pathname;
}

void
ra_unlink(const char *pathname)
{
        if (ra_strlen(pathname)) {
                (void)remove(pathname);
        }
}

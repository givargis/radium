//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_file.c
//

#include "ra_kernel.h"
#include "ra_file.h"

char *
ra_file_read(const char *pathname)
{
        const size_t PAD = 4;
        FILE *file;
        long size;
        char *s;

        assert( ra_strlen(pathname) );

        if (!(file = fopen(pathname, "r"))) {
                RA_TRACE("unable to open file");
                return NULL;
        }
        if (fseek(file, 0, SEEK_END) ||
            (0 > (size = ftell(file))) ||
            fseek(file, 0, SEEK_SET)) {
                fclose(file);
                RA_TRACE("unable to get file stat");
                return NULL;
        }
        if (!(s = ra_malloc(size + PAD))) {
                fclose(file);
                RA_TRACE("^");
                return NULL;
        }
        if (size && (1 != fread(s, size, 1, file))) {
                RA_FREE(s);
                fclose(file);
                RA_TRACE("file read failed");
                return NULL;
        }
        memset(s + size, 0, PAD);
        fclose(file);
        return s;
}

int
ra_file_write(const char *pathname, const char *s)
{
        FILE *file;

        assert( ra_strlen(pathname) );
        assert( s );

        if (!(file = fopen(pathname, "w"))) {
                RA_TRACE("unable to open file");
                return -1;
        }
        if (ra_strlen(s) && (1 != fwrite(s, ra_strlen(s), 1, file))) {
                fclose(file);
                RA_TRACE("file write failed");
                return -1;
        }
        fclose(file);
        return 0;
}

int
ra_file_test(void)
{
        const char *pathname, *s;

        // zero-byte write/read

        if (!(pathname = ra_pathname(NULL)) || ra_file_write(pathname, "")) {
                RA_FREE(pathname);
                RA_TRACE("^");
                return -1;
        }
        if (!(s = ra_file_read(pathname)) || ra_strlen(s)) {
                ra_unlink(pathname);
                RA_FREE(pathname);
                RA_FREE(s);
                RA_TRACE("software bug detected");
                return -1;
        }
        RA_FREE(s);

        // single-byte write/read

        if (ra_file_write(pathname, "x")) {
                RA_FREE(pathname);
                RA_TRACE("^");
                return -1;
        }
        if (!(s = ra_file_read(pathname)) ||
            (1 != ra_strlen(s)) ||
            ('x' != (*s))) {
                ra_unlink(pathname);
                RA_FREE(pathname);
                RA_FREE(s);
                RA_TRACE("software bug detected");
                return -1;
        }
        RA_FREE(s);

        // multi-byte write

        if (ra_file_write(pathname, pathname)) {
                RA_FREE(pathname);
                RA_TRACE("^");
                return -1;
        }
        if (!(s = ra_file_read(pathname)) ||
            (ra_strlen(pathname) != ra_strlen(s)) ||
            strcmp(pathname, s)) {
                ra_unlink(pathname);
                RA_FREE(pathname);
                RA_FREE(s);
                RA_TRACE("software bug detected");
                return -1;
        }
        ra_unlink(pathname);
        RA_FREE(pathname);
        RA_FREE(s);
        return 0;
}

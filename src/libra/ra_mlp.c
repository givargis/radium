//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_mlp.c
//

#include "ra_kernel.h"
#include "ra_base64.h"
#include "ra_file.h"
#include "ra_mlp.h"

struct ra_mlp {
        int input;
        int output;
        int hidden;
        int layers;
        struct {
                double *w;
                double *b;
                double *a_;
                double *d_;
                double *w_;
                double *b_;
        } *net;
};

static void
mac1(double *z, const double *a, const double *b, int n, int m)
{
        for (int i=0; i<n; ++i) {
                z[i] = 0.0;
                for (int j=0; j<m; ++j) {
                        z[i] += a[i * m + j] * b[j];
                }
        }
}

static void
mac2(double *z, const double *a, const double *b, int n, int m)
{
        for (int i=0; i<m; ++i) {
                z[i] = 0.0;
                for (int j=0; j<n; ++j) {
                        z[i] += a[j * m + i] * b[j];
                }
        }
}

static void
mac3(double *za, const double *b, const double *c, int n, int m)
{
        for (int i=0; i<n; ++i) {
                for (int j=0; j<m; ++j) {
                        za[i * m + j] += b[i] * c[j];
                }
        }
}

static void
mac4(double *za, const double *b, double s, int n)
{
        for (int i=0; i<n; ++i) {
                za[i] += b[i] * s;
        }
}

static void
add(double *za, const double *b, int n)
{
        for (int i=0; i<n; ++i) {
                za[i] += b[i];
        }
}

static void
sub(double *z, const double *a, const double *b, int n)
{
        for (int i=0; i<n; ++i) {
                z[i] = a[i] - b[i];
        }
}

static void
relu(double *za, int n)
{
        for (int i=0; i<n; ++i) {
                if (0.0 >= za[i]) {
                        za[i] = 0.0;
                }
        }
}

static void
relud(double *za, const double *b, int n)
{
        for (int i=0; i<n; ++i) {
                if (0.0 >= b[i]) {
                        za[i] = 0.0;
                }
        }
}

static int
size(const struct ra_mlp *mlp, int l)
{
        if (0 == l) {
                return mlp->input;
        }
        if (mlp->layers == (l + 1)) {
                return mlp->output;
        }
        return mlp->hidden;
}

static void
randomize(struct ra_mlp *mlp)
{
        for (int l=1; l<mlp->layers; ++l) {
                int n = size(mlp, l);
                int m = size(mlp, l - 1);
                double a = -sqrt(6.0 / (n * m)) * 1.0;
                double b = +sqrt(6.0 / (n * m)) * 2.0;
                for (int i=0; i<(n*m); ++i) {
                        mlp->net[l].w[i] = a + (rand() / (double)RAND_MAX) * b;
                }
        }
}

static void
activate(struct ra_mlp *mlp, const double *x)
{
        /**
         * a_[0] := x
         * a_[l] := activation( w[l] * a_[l - 1] + b[l] )
         *
         * activation:
         *    RELU - internal
         *    LINEAR - output
         */

        memcpy(mlp->net[0].a_, x, size(mlp, 0) * sizeof (mlp->net[0].a_[0]));
        for (int l=1; l<mlp->layers; ++l) {
                int n = size(mlp, l);
                int m = size(mlp, l - 1);
                mac1(mlp->net[l].a_, mlp->net[l].w, mlp->net[l - 1].a_, n, m);
                add(mlp->net[l].a_, mlp->net[l].b, n);
                if ((l + 1) < mlp->layers) {
                        relu(mlp->net[l].a_, n);
                }
        }
}

static void
backprop(struct ra_mlp *mlp, const double *y)
{
        int l, n, m;

        // start with last layer

        l = mlp->layers - 1;

        /**
         * Quadratic Cost Function
         *
         * d_[L] := a_[L] − y
         */

        sub(mlp->net[l].d_, mlp->net[l].a_, y, size(mlp, l));

        /**
         * d_[l] := (w[l+1]' * d_[l+1]) ⊙ σ′(a_[l])
         */

        while (1 < l) {
                n = size(mlp, l);
                m = size(mlp, l - 1);
                mac2(mlp->net[l - 1].d_, mlp->net[l].w, mlp->net[l].d_, n, m);
                relud(mlp->net[l - 1].d_, mlp->net[l - 1].a_, m);
                --l;
        }

        /**
         * b_[l] := b_[l] + d_[l]
         * w_[l] := w_[l] + d_[l] * a_[l - 1]
         */

        for (l=1; l<mlp->layers; ++l) {
                n = size(mlp, l);
                m = size(mlp, l - 1);
                add(mlp->net[l].b_, mlp->net[l].d_, n);
                mac3(mlp->net[l].w_, mlp->net[l].d_, mlp->net[l - 1].a_, n, m);
        }
}

static unsigned
argmax(const double *a, int n)
{
        int m;

        m = 0;
        for (int i=1; i<n; ++i) {
                if (a[m] < a[i]) {
                        m = i;
                }
        }
        return (unsigned)m;
}

ra_mlp_t
ra_mlp_open(int input, int output, int hidden, int layers)
{
        struct ra_mlp *mlp;
        int n, m;

        assert( (1 <= input) && (1000000 >= input) );
        assert( (1 <= output) && (1000000 >= output) );
        assert( (1 <= hidden) && (1000000 >= hidden) );
        assert( (3 <= layers) && (20 >= layers) );

        // initialize

        if (!(mlp = malloc(sizeof (struct ra_mlp)))) {
                RA_TRACE("out of memory");
                return 0;
        }
        memset(mlp, 0, sizeof (struct ra_mlp));
        mlp->input = input;
        mlp->output = output;
        mlp->hidden = hidden;
        mlp->layers = layers;

        // network

        if (!(mlp->net = malloc(mlp->layers * sizeof (mlp->net[0])))) {
                ra_mlp_close(mlp);
                RA_TRACE("out of memory");
                return 0;
        }
        memset(mlp->net, 0, mlp->layers * sizeof (mlp->net[0]));
        for (int l=0; l<mlp->layers; ++l) {
                n = size(mlp, l);
                if (!(mlp->net[l].a_ = malloc(n * sizeof (mlp->net[0].a_))) ||
                    !(mlp->net[l].d_ = malloc(n * sizeof (mlp->net[0].d_)))) {
                        ra_mlp_close(mlp);
                        RA_TRACE("out of memory");
                        return 0;
                }
                memset(mlp->net[l].a_, 0, n * sizeof (mlp->net[0].a_));
                memset(mlp->net[l].d_, 0, n * sizeof (mlp->net[0].d_));
        }
        for (int l=1; l<mlp->layers; ++l) {
                n = size(mlp, l);
                m = size(mlp, l - 1);
                mlp->net[l].w = malloc(n * m * sizeof (mlp->net[0].w));
                mlp->net[l].b = malloc(n * 1 * sizeof (mlp->net[0].w));
                mlp->net[l].w_ = malloc(n * m * sizeof (mlp->net[0].w));
                mlp->net[l].b_ = malloc(n * 1 * sizeof (mlp->net[0].w));
                if (!mlp->net[l].w ||
                    !mlp->net[l].b ||
                    !mlp->net[l].w_ ||
                    !mlp->net[l].b_) {
                        ra_mlp_close(mlp);
                        RA_TRACE("out of memory");
                        return 0;
                }
                memset(mlp->net[l].w, 0, n * m * sizeof (mlp->net[0].w));
                memset(mlp->net[l].b, 0, n * 1 * sizeof (mlp->net[0].b));
                memset(mlp->net[l].w_, 0, n * m * sizeof (mlp->net[0].w_));
                memset(mlp->net[l].b_, 0, n * 1 * sizeof (mlp->net[0].b_));
        }
        randomize(mlp);
        return mlp;
}

void
ra_mlp_close(ra_mlp_t mlp)
{
        if (mlp) {
                if (mlp->net) {
                        for (int l=0; l<mlp->layers; ++l) {
                                RA_FREE(mlp->net[l].a_);
                                RA_FREE(mlp->net[l].d_);
                        }
                        for (int l=1; l<mlp->layers; ++l) {
                                RA_FREE(mlp->net[l].w);
                                RA_FREE(mlp->net[l].b);
                                RA_FREE(mlp->net[l].w_);
                                RA_FREE(mlp->net[l].b_);
                        }
                }
                RA_FREE(mlp->net);
                memset(mlp, 0, sizeof (struct ra_mlp));
        }
        RA_FREE(mlp);
}

const double *
ra_mlp_activate(ra_mlp_t mlp, const double *x)
{
        activate(mlp, x);
        return mlp->net[mlp->layers - 1].a_;
}

void
ra_mlp_train(ra_mlp_t mlp,
             const double *x,
             const double *y,
             double learning_rate,
             int batch_size)
{
        int n, m;

        assert( (0.0 < learning_rate) && (1.0 >= learning_rate) );
        assert( (1 <= batch_size) && (1024 >= batch_size) );

        /**
         * w_[*] := 0.0
         * b_[*] := 0.0
         */

        for (int l=1; l<mlp->layers; ++l) {
                n = size(mlp, l);
                m = size(mlp, l - 1);
                memset(mlp->net[l].w_, 0, n * m * sizeof (mlp->net[0].w[0]));
                memset(mlp->net[l].b_, 0, n * 1 * sizeof (mlp->net[0].b[0]));
        }

        /**
         * for all (x -> y):
         *   activate()
         *   backprop()
         */

        for (int i=0; i<batch_size; ++i) {
                activate(mlp, x + i * mlp->input);
                backprop(mlp, y + i * mlp->output);
        }

        /**
         * w[l] := w[l] - ( (η / batch_size) * w_[l] )
         * b[l] := b[l] - ( (η / batch_size) * b_[l] )
         */

        for (int l=1; l<mlp->layers; ++l) {
                n = size(mlp, l);
                m = size(mlp, l - 1);
                mac4(mlp->net[l].w,
                     mlp->net[l].w_,
                     -learning_rate / batch_size,
                     n * m);
                mac4(mlp->net[l].b,
                     mlp->net[l].b_,
                     -learning_rate / batch_size,
                     n * 1);
        }
}

int
ra_mlp_load(ra_mlp_t mlp, const char *pathname)
{
        size_t sw, sb;
        FILE *file;
        int n, m;

        assert( ra_strlen(pathname) );

        if (!(file = fopen(pathname, "rb"))) {
                RA_TRACE("unable to open file for reading");
                return -1;
        }
        for (int l=1; l<mlp->layers; ++l) {
                n = size(mlp, l);
                m = size(mlp, l - 1);
                sw = n * m * sizeof (mlp->net[0].w);
                sb = n * 1 * sizeof (mlp->net[0].b);
                if ((sw != fread(mlp->net[l].w, 1, sw, file)) ||
                    (sb != fread(mlp->net[l].b, 1, sb, file))) {
                        ra_unlink(pathname);
                        RA_TRACE("unable to read file");
                        return -1;
                }
        }
        fclose(file);
        return 0;
}

int
ra_mlp_store(ra_mlp_t mlp, const char *pathname)
{
        size_t sw, sb;
        FILE *file;
        int n, m;

        assert( ra_strlen(pathname) );

        if (!(file = fopen(pathname, "wb"))) {
                RA_TRACE("unable to open file for writing");
                return -1;
        }
        for (int l=1; l<mlp->layers; ++l) {
                n = size(mlp, l);
                m = size(mlp, l - 1);
                sw = n * m * sizeof (mlp->net[0].w);
                sb = n * 1 * sizeof (mlp->net[0].b);
                if ((sw != fwrite(mlp->net[l].w, 1, sw, file)) ||
                    (sb != fwrite(mlp->net[l].b, 1, sb, file))) {
                        ra_unlink(pathname);
                        RA_TRACE("unable to write file");
                        return -1;
                }
        }
        fclose(file);
        return 0;
}

int
ra_mlp_test(void)
{
        const char *IMAGES_PATHNAME = "../data/images.dat";
        const char *LABELS_PATHNAME = "../data/labels.dat";
        const int TRAIN_N = 60000;
        const int TEST_N = 10000;
        const int BATCH_SIZE = 8;
        const char *s1, *s2;
        uint8_t *images;
        uint8_t *labels;
        size_t n1, n2;
        double *x, *y;
        ra_mlp_t mlp;
        int errors;

        // load MNIST data

        if (!(s1 = ra_file_read(IMAGES_PATHNAME)) ||
            !(s2 = ra_file_read(LABELS_PATHNAME))) {
                RA_FREE(s1);
                RA_TRACE("^");
                return -1;
        }
        images = labels = NULL;
        if (!(images = malloc(RA_BASE64_DECODE_LEN(ra_strlen(s1)))) ||
            !(labels = malloc(RA_BASE64_DECODE_LEN(ra_strlen(s2))))) {
                RA_FREE(s1);
                RA_FREE(s2);
                RA_FREE(images);
                RA_FREE(labels);
                RA_TRACE("out of memory");
                return -1;
        }
        if (ra_base64_decode(images, &n1, s1) ||
            ra_base64_decode(labels, &n2, s2)) {
                RA_FREE(s1);
                RA_FREE(s2);
                RA_FREE(images);
                RA_FREE(labels);
                RA_TRACE("^");
                return -1;
        }
        RA_FREE(s1);
        RA_FREE(s2);

        // sanity check

        assert( (size_t)(TRAIN_N + TEST_N) * 28 * 28 == n1 );
        assert( (size_t)(TRAIN_N + TEST_N) *  1 *  1 == n2 );

        // build model

        if (!(mlp = ra_mlp_open(28 * 28, 10, 50, 4))) {
                RA_FREE(images);
                RA_FREE(labels);
                RA_TRACE("^");
                return -1;
        }

        // initialize

        if (!(x = malloc(BATCH_SIZE * 28 * 28 * sizeof (x[0]))) ||
            !(y = malloc(BATCH_SIZE *  1 * 10 * sizeof (y[0])))) {
                RA_FREE(x);
                RA_FREE(images);
                RA_FREE(labels);
                ra_mlp_close(mlp);
                RA_TRACE("out of memory");
                return -1;
        }

        // train

        for (int i=0; i<(TRAIN_N/BATCH_SIZE); ++i) {
                for (int j=0; j<BATCH_SIZE; ++j) {
                        for (int k=0; k<(28*28); ++k) {
                                x[j * (28 * 28) + k] = (*(images++)) / 255.0;
                        }
                        for (int k=0; k<10; ++k) {
                                y[j * 10 + k] = 0.0;
                        }
                        y[j * 10 + (*(labels++))] = 1.0;
                }
                ra_mlp_train(mlp, x, y, 0.1, BATCH_SIZE);
        }

        // store/load

        if (ra_mlp_store(mlp, "mnist.mlp") || ra_mlp_load(mlp, "mnist.mlp")) {
                RA_TRACE("^");
                return -1;
        }
        ra_unlink("mnist.mlp");

        // test

        errors = 0;
        for (int i=0; i<TEST_N; ++i) {
                for (int k=0; k<(28*28); ++k) {
                        x[k] = (*(images++)) / 255.0;
                }
                if (argmax(ra_mlp_activate(mlp, x), 10) != (*(labels++))) {
                        ++errors;
                }
        }

        // cleanup

        images -= (TRAIN_N + TEST_N) * 28 * 28;
        labels -= (TRAIN_N + TEST_N) *  1 *  1;
        RA_FREE(x);
        RA_FREE(y);
        RA_FREE(images);
        RA_FREE(labels);
        ra_mlp_close(mlp);

        // verify

        if ((TEST_N * 0.05) < errors) {
                RA_TRACE("software bug detected");
                return -1;
        }
        return 0;
}

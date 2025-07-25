/**
 * Copyright (c) Tony Givargis, 2024-2025
 *
 * ra_ann.c
 */

#include "ra_base64.h"
#include "ra_ann.h"

struct ra_ann {
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
size(const struct ra_ann *ann, int l)
{
	if (0 == l) {
		return ann->input;
	}
	if (ann->layers == (l + 1)) {
		return ann->output;
	}
	return ann->hidden;
}

static void
randomize(struct ra_ann *ann)
{
	for (int l=1; l<ann->layers; ++l) {
		int n = size(ann, l);
		int m = size(ann, l - 1);
		double a = -sqrt(6.0 / (n * m)) * 1.0;
		double b = +sqrt(6.0 / (n * m)) * 2.0;
		for (int i=0; i<(n*m); ++i) {
			ann->net[l].w[i] = a + (rand() / (double)RAND_MAX) * b;
		}
	}
}

static void
activate(struct ra_ann *ann, const double *x)
{
	/**
	 * a_[0] := x
	 * a_[l] := activation( w[l] * a_[l - 1] + b[l] )
	 *
	 * activation:
	 *    RELU - internal
	 *    LINEAR - output
	 */

	memcpy(ann->net[0].a_, x, size(ann, 0) * sizeof (ann->net[0].a_[0]));
	for (int l=1; l<ann->layers; ++l) {
		int n = size(ann, l);
		int m = size(ann, l - 1);
		mac1(ann->net[l].a_, ann->net[l].w, ann->net[l - 1].a_, n, m);
		add(ann->net[l].a_, ann->net[l].b, n);
		if ((l + 1) < ann->layers) {
			relu(ann->net[l].a_, n);
		}
	}
}

static void
backprop(struct ra_ann *ann, const double *y)
{
	int l, n, m;

	// start with last layer

	l = ann->layers - 1;

	/**
	 * Quadratic Cost Function
	 *
	 * d_[L] := a_[L] − y
	 */

	sub(ann->net[l].d_, ann->net[l].a_, y, size(ann, l));

	/**
	 * d_[l] := (w[l+1]' * d_[l+1]) ⊙ σ′(a_[l])
	 */

	while (1 < l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		mac2(ann->net[l - 1].d_, ann->net[l].w, ann->net[l].d_, n, m);
		relud(ann->net[l - 1].d_, ann->net[l - 1].a_, m);
		--l;
	}

	/**
	 * b_[l] := b_[l] + d_[l]
	 * w_[l] := w_[l] + d_[l] * a_[l - 1]
	 */

	for (l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		add(ann->net[l].b_, ann->net[l].d_, n);
		mac3(ann->net[l].w_, ann->net[l].d_, ann->net[l - 1].a_, n, m);
	}
}

static int
argmax(const double *a, int n)
{
	int m;

	m = 0;
	for (int i=1; i<n; ++i) {
		if (a[m] < a[i]) {
			m = i;
		}
	}
	return m;
}

ra_ann_t
ra_ann_open(int input, int output, int hidden, int layers)
{
	struct ra_ann *ann;
	int n, m;

	assert( (1 <= input) && (1000000 >= input) );
	assert( (1 <= output) && (1000000 >= output) );
	assert( (1 <= hidden) && (1000000 >= hidden) );
	assert( (3 <= layers) && (20 >= layers) );

	// initialize

	if (!(ann = malloc(sizeof (struct ra_ann)))) {
		RA_TRACE("out of memory");
		return 0;
	}
	memset(ann, 0, sizeof (struct ra_ann));
	ann->input = input;
	ann->output = output;
	ann->hidden = hidden;
	ann->layers = layers;

	// initialize (network)

	if (!(ann->net = malloc(ann->layers * sizeof (ann->net[0])))) {
		ra_ann_close(ann);
		RA_TRACE("out of memory");
		return 0;
	}
	memset(ann->net, 0, ann->layers * sizeof (ann->net[0]));
	for (int l=0; l<ann->layers; ++l) {
		n = size(ann, l);
		if (!(ann->net[l].a_ = malloc(n * sizeof (ann->net[0].a_))) ||
		    !(ann->net[l].d_ = malloc(n * sizeof (ann->net[0].d_)))) {
			ra_ann_close(ann);
			RA_TRACE("out of memory");
			return 0;
		}
		memset(ann->net[l].a_, 0, n * sizeof (ann->net[0].a_));
		memset(ann->net[l].d_, 0, n * sizeof (ann->net[0].d_));
	}
	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		ann->net[l].w = malloc(n * m * sizeof (ann->net[0].w));
		ann->net[l].b = malloc(n * 1 * sizeof (ann->net[0].w));
		ann->net[l].w_ = malloc(n * m * sizeof (ann->net[0].w));
		ann->net[l].b_ = malloc(n * 1 * sizeof (ann->net[0].w));
		if (!ann->net[l].w ||
		    !ann->net[l].b ||
		    !ann->net[l].w_ ||
		    !ann->net[l].b_) {
			ra_ann_close(ann);
			RA_TRACE("out of memory");
			return 0;
		}
		memset(ann->net[l].w, 0, n * m * sizeof (ann->net[0].w));
		memset(ann->net[l].b, 0, n * 1 * sizeof (ann->net[0].b));
		memset(ann->net[l].w_, 0, n * m * sizeof (ann->net[0].w_));
		memset(ann->net[l].b_, 0, n * 1 * sizeof (ann->net[0].b_));
	}
	randomize(ann);
	return ann;
}

void
ra_ann_close(ra_ann_t ann)
{
	if (ann) {
		if (ann->net) {
			for (int l=0; l<ann->layers; ++l) {
				free(ann->net[l].a_);
				free(ann->net[l].d_);
			}
			for (int l=1; l<ann->layers; ++l) {
				free(ann->net[l].w);
				free(ann->net[l].b);
				free(ann->net[l].w_);
				free(ann->net[l].b_);
			}
		}
		free(ann->net);
		memset(ann, 0, sizeof (struct ra_ann));
	}
	free(ann);
}

const double *
ra_ann_activate(ra_ann_t ann, const double *x)
{
	assert( ann );
	assert( x );

	activate(ann, x);
	return ann->net[ann->layers - 1].a_;
}

void
ra_ann_train(ra_ann_t ann,
	     const double *x,
	     const double *y,
	     double learning_rate,
	     int batch_size)
{
	int n, m;

	assert( ann );
	assert( x );
	assert( y );
	assert( (0.0 < learning_rate) && (1.0 >= learning_rate) );
	assert( (1 <= batch_size) && (1024 >= batch_size) );

	/**
	 * w_[*] := 0.0
	 * b_[*] := 0.0
	 */

	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		memset(ann->net[l].w_, 0, n * m * sizeof (ann->net[0].w[0]));
		memset(ann->net[l].b_, 0, n * 1 * sizeof (ann->net[0].b[0]));
	}

	/**
	 * for all (x -> y):
	 *   activate()
	 *   backprop()
	 */

	for (int i=0; i<batch_size; ++i) {
		activate(ann, x + i * ann->input);
		backprop(ann, y + i * ann->output);
	}

	/**
	 * w[l] := w[l] - ( (η / batch_size) * w_[l] )
	 * b[l] := b[l] - ( (η / batch_size) * b_[l] )
	 */

	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		mac4(ann->net[l].w,
		     ann->net[l].w_,
		     -learning_rate / batch_size,
		     n * m);
		mac4(ann->net[l].b,
		     ann->net[l].b_,
		     -learning_rate / batch_size,
		     n * 1);
	}
}

int
ra_ann_bist(void)
{
	const int TRAIN_N = 60000;
	const int TEST_N = 10000;
	const int BATCH_SIZE = 8;
	const char *s1, *s2;
	const char *s3, *s4;
	uint8_t *images;
	uint8_t *labels;
	size_t n1, n2;
	double *x, *y;
	ra_ann_t ann;
	int errors;

	// load MNIST data

	s1 = s2 = s3 = s4 = NULL;
	if (!(s1 = ra_pathname(RA_DIRECTORY_DATA, "images.dat")) ||
	    !(s2 = ra_pathname(RA_DIRECTORY_DATA, "labels.dat")) ||
	    !(s3 = ra_file_read(s1)) ||
	    !(s4 = ra_file_read(s2))) {
		free((void *)s1);
		free((void *)s2);
		free((void *)s3);
		free((void *)s4);
		RA_TRACE(NULL);
		return -1;
	}
	free((void *)s1);
	free((void *)s2);
	images = labels = NULL;
	if (!(images = malloc(RA_BASE64_DECODE_LEN(strlen(s3)))) ||
	    !(labels = malloc(RA_BASE64_DECODE_LEN(strlen(s4))))) {
		free((void *)s3);
		free((void *)s4);
		free(images);
		free(labels);
		RA_TRACE("out of memory");
		return -1;
	}
	if (ra_base64_decode(images, &n1, s3) ||
	    ra_base64_decode(labels, &n2, s4)) {
		free((void *)s3);
		free((void *)s4);
		free(images);
		free(labels);
		RA_TRACE(NULL);
		return -1;
	}
	free((void *)s3);
	free((void *)s4);

	// sanity check

	assert( (TRAIN_N + TEST_N) * 28 * 28 == n1 );
	assert( (TRAIN_N + TEST_N) *  1 *  1 == n2 );

	// build model

	if (!(ann = ra_ann_open(28 * 28, 10, 100, 4))) {
		free(images);
		free(labels);
		RA_TRACE(NULL);
		return -1;
	}

	// initialize

	if (!(x = malloc(BATCH_SIZE * 28 * 28 * sizeof (x[0]))) ||
	    !(y = malloc(BATCH_SIZE *  1 * 10 * sizeof (y[0])))) {
		free(x);
		free(images);
		free(labels);
		ra_ann_close(ann);
		RA_TRACE("out of memory");
		return -1;
	}

	// train

	for (int i=0; i<(TRAIN_N/BATCH_SIZE); ++i) {
		for (int j=0; j<BATCH_SIZE; ++j) {
			for (int k=0; k<(28*28); ++k) {
				x[j * (28 * 28) + k] = (*images++) / 255.0;
			}
			for (int k=0; k<10; ++k) {
				y[j * 10 + k] = 0.0;
			}
			y[j * 10 + (*labels++)] = 1.0;
		}
		ra_ann_train(ann, x, y, 0.1, BATCH_SIZE);
	}

	// test

	errors = 0;
	for (int i=0; i<TEST_N; ++i) {
		for (int k=0; k<(28*28); ++k) {
			x[k] = (*images++) / 255.0;
		}
		if (argmax(ra_ann_activate(ann, x), 10) != (int)(*labels++)) {
			++errors;
		}
	}

	// cleanup

	images -= (TRAIN_N + TEST_N) * 28 * 28;
	labels -= (TRAIN_N + TEST_N) *  1 *  1;
	free(x);
	free(y);
	free(images);
	free(labels);
	ra_ann_close(ann);

	// verify

	if ((TEST_N * 0.05) < errors) {
		RA_TRACE("software");
		return -1;
	}
	return 0;
}

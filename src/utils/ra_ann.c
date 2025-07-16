/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_ann.c
 */

#include "ra_ann.h"

struct ra__ann {
	int input;
	int output;
	int hidden;
	int layers;
	ra__real_t error;
	struct {
		ra__real_t *w;
		ra__real_t *b;
		ra__real_t *a_;
		ra__real_t *d_;
		ra__real_t *w_;
		ra__real_t *b_;
	} *net;
};

static void
mac1(ra__real_t *z, const ra__real_t *a, const ra__real_t *b, int n, int m)
{
	for (int i=0; i<n; ++i) {
		z[i] = 0.0;
		for (int j=0; j<m; ++j) {
			z[i] += a[i * m + j] * b[j];
		}
	}
}

static void
mac2(ra__real_t *z, const ra__real_t *a, const ra__real_t *b, int n, int m)
{
	for (int i=0; i<m; ++i) {
		z[i] = 0.0;
		for (int j=0; j<n; ++j) {
			z[i] += a[j * m + i] * b[j];
		}
	}
}

static void
mac3(ra__real_t *za, const ra__real_t *b, const ra__real_t *c, int n, int m)
{
	for (int i=0; i<n; ++i) {
		for (int j=0; j<m; ++j) {
			za[i * m + j] += b[i] * c[j];
		}
	}
}

static void
mac4(ra__real_t *za, const ra__real_t *b, ra__real_t s, int n)
{
	for (int i=0; i<n; ++i) {
		za[i] += b[i] * s;
	}
}

static void
add(ra__real_t *za, const ra__real_t *b, int n)
{
	for (int i=0; i<n; ++i) {
		za[i] += b[i];
	}
}

static void
sub(ra__real_t *z, const ra__real_t *a, const ra__real_t *b, int n)
{
	for (int i=0; i<n; ++i) {
		z[i] = a[i] - b[i];
	}
}

static void
relu(ra__real_t *za, int n)
{
	for (int i=0; i<n; ++i) {
		if (0.0 >= za[i]) {
			za[i] = 0.0;
		}
	}
}

static void
relud(ra__real_t *za, const ra__real_t *b, int n)
{
	for (int i=0; i<n; ++i) {
		if (0.0 >= b[i]) {
			za[i] = 0.0;
		}
	}
}

static int
size(const struct ra__ann *ann, int l)
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
randomize(struct ra__ann *ann)
{
	ra__real_t a, b;
	int n, m;

	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		a = -sqrt(6.0 / (n * m)) * 1.0;
		b = +sqrt(6.0 / (n * m)) * 2.0;
		for (int i=0; i<(n*m); ++i) {
			ann->net[l].w[i]  = a;
			ann->net[l].w[i] += (rand()/(ra__real_t)RAND_MAX) * b;
		}
	}
}

static void
activate_(struct ra__ann *ann, const ra__real_t *x)
{
	int n, m;

	/*
	 * a_[0] := x
	 * a_[l] := activation( w[l] * a_[l - 1] + b[l] )
	 *
	 * activation:
	 *    RELU - internal
	 *    LINEAR - output
	 */

	memcpy(ann->net[0].a_, x, size(ann, 0) * sizeof (ann->net[0].a_[0]));
	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		mac1(ann->net[l].a_, ann->net[l].w, ann->net[l - 1].a_, n, m);
		add(ann->net[l].a_, ann->net[l].b, n);
		if ((l + 1) < ann->layers) {
			relu(ann->net[l].a_, n);
		}
	}
}

static void
backprop_(struct ra__ann *ann, const ra__real_t *y)
{
	int l, n, m;

	/*
	 * start with last layer
	 */

	l = ann->layers - 1;

	/*
	 * using: Quadratic Cost Function
	 *
	 * d_[L] := a_[L] − y
	 */

	sub(ann->net[l].d_, ann->net[l].a_, y, size(ann, l));

	/*
	 * track error
	 */

	n = size(ann, l);
	for (int i=0; i<n; ++i) {
		ann->error += ann->net[l].d_[i] * ann->net[l].d_[i];
	}

	/*
	 * d_[l] := (w[l+1]' * d_[l+1]) ⊙ σ′(a_[l])
	 */

	while (1 < l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		mac2(ann->net[l - 1].d_, ann->net[l].w, ann->net[l].d_, n, m);
		relud(ann->net[l - 1].d_, ann->net[l - 1].a_, m);
		--l;
	}

	/*
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

ra__ann_t
ra__ann_open(int input, int output, int hidden, int layers)
{
	struct ra__ann *ann;
	int n, m;

	assert( (1 <= input) && (1000000 >= input) );
	assert( (1 <= output) && (1000000 >= output) );
	assert( (1 <= hidden) && (1000000 >= hidden) );
	assert( (3 <= layers) && (20 >= layers) );

	// initialize

	if (!(ann = ra__malloc(sizeof (struct ra__ann)))) {
		RA__ERROR_TRACE(0);
		return 0;
	}
	memset(ann, 0, sizeof (struct ra__ann));
	ann->input = input;
	ann->output = output;
	ann->hidden = hidden;
	ann->layers = layers;

	// initialize

	if (!(ann->net = ra__malloc(ann->layers * sizeof (ann->net[0])))) {
		ra__ann_close(ann);
		RA__ERROR_TRACE(0);
		return 0;
	}

	// initialize

	for (int l=0; l<ann->layers; ++l) {
		n = size(ann, l);
		ann->net[l].a_ = ra__malloc(n * sizeof (ann->net[0].a_));
		ann->net[l].d_ = ra__malloc(n * sizeof (ann->net[0].d_));
		if (!ann->net[l].a_ || !ann->net[l].d_) {
			ra__ann_close(ann);
			RA__ERROR_TRACE(0);
			return 0;
		}
		memset(ann->net[l].a_, 0, n * sizeof (ann->net[0].a_));
		memset(ann->net[l].d_, 0, n * sizeof (ann->net[0].d_));
	}

	// initialize

	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		ann->net[l].w = ra__malloc(n * m * sizeof (ann->net[0].w));
		ann->net[l].b = ra__malloc(n * 1 * sizeof (ann->net[0].w));
		ann->net[l].w_ = ra__malloc(n * m * sizeof (ann->net[0].w));
		ann->net[l].b_ = ra__malloc(n * 1 * sizeof (ann->net[0].w));
		if (!ann->net[l].w ||
		    !ann->net[l].b ||
		    !ann->net[l].w_ ||
		    !ann->net[l].b_) {
			ra__ann_close(ann);
			RA__ERROR_TRACE(0);
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
ra__ann_close(ra__ann_t ann)
{
	if (ann) {
		if (ann->net) {
			for (int l=0; l<ann->layers; ++l) {
				RA__FREE(ann->net[l].a_);
				RA__FREE(ann->net[l].d_);
			}
			for (int l=1; l<ann->layers; ++l) {
				RA__FREE(ann->net[l].w);
				RA__FREE(ann->net[l].b);
				RA__FREE(ann->net[l].w_);
				RA__FREE(ann->net[l].b_);
			}
		}
		RA__FREE(ann->net);
		memset(ann, 0, sizeof (struct ra__ann));
	}
	RA__FREE(ann);
}

const ra__real_t *
ra__ann_activate(ra__ann_t ann, const ra__real_t *x)
{
	assert( ann );
	assert( x );

	activate_(ann, x);
	return ann->net[ann->layers - 1].a_;
}

ra__real_t
ra__ann_train(ra__ann_t ann,
	      const ra__real_t *x,
	      const ra__real_t *y,
	      ra__real_t eta,
	      int k)
{
	int n, m;

	assert( ann );
	assert( x && y );
	assert( (0.0 < eta) && (1.0 >= eta) );
	assert( (1 <= k) && (128 >= k) );

	/*
	 * track error
	 */

	ann->error = 0.0;

	/*
	 * w_[*] := 0.0
	 * b_[*] := 0.0
	 */

	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		memset(ann->net[l].w_, 0, n * m * sizeof (ann->net[0].w[0]));
		memset(ann->net[l].b_, 0, n * 1 * sizeof (ann->net[0].b[0]));
	}

	/*
	 * for all (x -> y):
	 *   activate()
	 *   backprop()
	 */

	for (int i=0; i<k; ++i) {
		activate_(ann, x + i * ann->input);
		backprop_(ann, y + i * ann->output);
	}

	/*
	 * w[l] := w[l] - ( (η / k) * w_[l] )
	 * b[l] := b[l] - ( (η / k) * b_[l] )
	 */

	for (int l=1; l<ann->layers; ++l) {
		n = size(ann, l);
		m = size(ann, l - 1);
		mac4(ann->net[l].w, ann->net[l].w_, -eta / k, n * m);
		mac4(ann->net[l].b, ann->net[l].b_, -eta / k, n * 1);
	}
	return sqrt(ann->error);
}

int
ra__ann_bist(void)
{
	return 0;
}

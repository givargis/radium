/* Copyright (c) Tony Givargis, 2024-2026 */

#include "ra_mlp.h"

struct ra_mlp {
	int input;
	int output;
	int hidden;
	int layers;
	struct {
		float *w;
		float *b;
		float *a_;
		float *d_;
		float *w_;
		float *b_;
	} *net;
};

static void
mac1(float *z, const float *a, const float *b, int n, int m)
{
	int i, j;

	for (i=0; i<n; ++i) {
		z[i] = 0.0;
		for (j=0; j<m; ++j) {
			z[i] += a[i * m + j] * b[j];
		}
	}
}

static void
mac2(float *z, const float *a, const float *b, int n, int m)
{
	int i, j;

	for (i=0; i<m; ++i) {
		z[i] = 0.0;
		for (j=0; j<n; ++j) {
			z[i] += a[j * m + i] * b[j];
		}
	}
}

static void
mac3(float *za, const float *b, const float *c, int n, int m)
{
	int i, j;

	for (i=0; i<n; ++i) {
		for (j=0; j<m; ++j) {
			za[i * m + j] += b[i] * c[j];
		}
	}
}

static void
mac4(float *za, const float *b, float s, int n)
{
	int i;

	for (i=0; i<n; ++i) {
		za[i] += b[i] * s;
	}
}

static void
add(float *za, const float *b, int n)
{
	int i;

	for (i=0; i<n; ++i) {
		za[i] += b[i];
	}
}

static void
sub(float *z, const float *a, const float *b, int n)
{
	int i;

	for (i=0; i<n; ++i) {
		z[i] = a[i] - b[i];
	}
}

static void
relu(float *za, int n)
{
	int i;

	for (i=0; i<n; ++i) {
		if (0.0 >= za[i]) {
			za[i] = 0.0;
		}
	}
}

static void
relud(float *za, const float *b, int n)
{
	int i;

	for (i=0; i<n; ++i) {
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
	int l, i;

	for (l=1; l<mlp->layers; ++l) {
		int n = size(mlp, l);
		int m = size(mlp, l - 1);
		float limit = sqrt(6.0 / (n + m));
		for (i=0; i<(n*m); ++i) {
			mlp->net[l].w[i]  = (rand() / (float)RAND_MAX);
			mlp->net[l].w[i] *= (2.0 * limit);
			mlp->net[l].w[i] -= limit;
		}
	}
}

static void
activate(struct ra_mlp *mlp, const float *x)
{
	int l;

	/**
	 * a_[0] := x
	 * a_[l] := activation( w[l] * a_[l - 1] + b[l] )
	 *
	 * activation:
	 *    RELU - internal
	 *    LINEAR - output
	 */

	memcpy(mlp->net[0].a_, x, size(mlp, 0) * sizeof (mlp->net[0].a_[0]));
	for (l=1; l<mlp->layers; ++l) {
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
backprop(struct ra_mlp *mlp, const float *y)
{
	int l, n, m;

	/**
	 * d_[*] := 0
	 */

	for (l=0; l<mlp->layers; ++l) {
		n = size(mlp, l);
		memset(mlp->net[l].d_, 0, n * sizeof (mlp->net[l].d_[0]));
	}

	/* last layer */

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

	for (; l>0; --l) {
		n = size(mlp, l);
		m = size(mlp, l - 1);
		mac2(mlp->net[l - 1].d_, mlp->net[l].w, mlp->net[l].d_, n, m);
		if (1 < l) {
			relud(mlp->net[l - 1].d_, mlp->net[l - 1].a_, m);
		}
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

ra_mlp_t
ra_mlp_open(int input, int output, int hidden, int layers)
{
	struct ra_mlp *mlp;
	int l, n, m;

	assert( (1 <= input) && (1000000 >= input) );
	assert( (1 <= output) && (1000000 >= output) );
	assert( (1 <= hidden) && (1000000 >= hidden) );
	assert( (3 <= layers) && (20 >= layers) );

	/* initialize */

	if (!(mlp = malloc(sizeof (struct ra_mlp)))) {
		RA_TRACE("out of memory");
		return 0;
	}
	memset(mlp, 0, sizeof (struct ra_mlp));
	mlp->input = input;
	mlp->output = output;
	mlp->hidden = hidden;
	mlp->layers = layers;

	/* network */

	if (!(mlp->net = malloc(mlp->layers * sizeof (mlp->net[0])))) {
		ra_mlp_close(mlp);
		RA_TRACE("out of memory");
		return 0;
	}
	memset(mlp->net, 0, mlp->layers * sizeof (mlp->net[0]));
	for (l=0; l<mlp->layers; ++l) {
		n = size(mlp, l);
		mlp->net[l].a_ = malloc(n * sizeof (mlp->net[0].a_[0]));
		mlp->net[l].d_ = malloc(n * sizeof (mlp->net[0].d_[0]));
		if (!mlp->net[l].a_ || !mlp->net[l].d_) {
			ra_mlp_close(mlp);
			RA_TRACE("out of memory");
			return 0;
		}
		memset(mlp->net[l].a_, 0, n * sizeof (mlp->net[0].a_[0]));
		memset(mlp->net[l].d_, 0, n * sizeof (mlp->net[0].d_[0]));
	}
	for (l=1; l<mlp->layers; ++l) {
		n = size(mlp, l);
		m = size(mlp, l - 1);
		mlp->net[l].w = malloc(n * m * sizeof (mlp->net[0].w[0]));
		mlp->net[l].b = malloc(n * 1 * sizeof (mlp->net[0].w[0]));
		mlp->net[l].w_ = malloc(n * m * sizeof (mlp->net[0].w[0]));
		mlp->net[l].b_ = malloc(n * 1 * sizeof (mlp->net[0].w[0]));
		if (!mlp->net[l].w ||
		    !mlp->net[l].b ||
		    !mlp->net[l].w_ ||
		    !mlp->net[l].b_) {
			ra_mlp_close(mlp);
			RA_TRACE("out of memory");
			return 0;
		}
		memset(mlp->net[l].w, 0, n * m * sizeof (mlp->net[0].w[0]));
		memset(mlp->net[l].b, 0, n * 1 * sizeof (mlp->net[0].b[0]));
		memset(mlp->net[l].w_, 0, n * m * sizeof (mlp->net[0].w_[0]));
		memset(mlp->net[l].b_, 0, n * 1 * sizeof (mlp->net[0].b_[0]));
	}
	randomize(mlp);
	return mlp;
}

void
ra_mlp_close(ra_mlp_t mlp)
{
	int l;

	if (mlp) {
		if (mlp->net) {
			for (l=0; l<mlp->layers; ++l) {
				RA_FREE(mlp->net[l].a_);
				RA_FREE(mlp->net[l].d_);
			}
			for (l=1; l<mlp->layers; ++l) {
				RA_FREE(mlp->net[l].w);
				RA_FREE(mlp->net[l].b);
				RA_FREE(mlp->net[l].w_);
				RA_FREE(mlp->net[l].b_);
			}
		}
		RA_FREE(mlp->net);
		memset(mlp, 0, sizeof (struct ra_mlp));
		RA_FREE(mlp);
	}
}

const float *
ra_mlp_activate(ra_mlp_t mlp, const float *x)
{
	assert( mlp && x );

	activate(mlp, x);
	return mlp->net[mlp->layers - 1].a_;
}

void
ra_mlp_train(ra_mlp_t mlp,
	     const float *x,
	     const float *y,
	     float learning_rate,
	     int batch_size)
{
	int l, n, m, i;

	assert( mlp && x && y );

	assert( (0.0 < learning_rate) && (1.0 >= learning_rate) );
	assert( (1 <= batch_size) && (1024 >= batch_size) );

	/**
	 * w_[*] := 0.0
	 * b_[*] := 0.0
	 */

	for (l=1; l<mlp->layers; ++l) {
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

	for (i=0; i<batch_size; ++i) {
		activate(mlp, x + i * mlp->input);
		backprop(mlp, y + i * mlp->output);
	}

	/**
	 * w[l] := w[l] - ( (η / batch_size) * w_[l] )
	 * b[l] := b[l] - ( (η / batch_size) * b_[l] )
	 */

	for (l=1; l<mlp->layers; ++l) {
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
	int l, n, m;

	assert( mlp && pathname && strlen(pathname) );

	if (!(file = fopen(pathname, "rb"))) {
		RA_TRACE("unable to open file for reading");
		return -1;
	}
	for (l=1; l<mlp->layers; ++l) {
		n = size(mlp, l);
		m = size(mlp, l - 1);
		sw = (size_t)n * m * sizeof (mlp->net[0].w[0]);
		sb = (size_t)n * 1 * sizeof (mlp->net[0].b[0]);
		if ((sw != fread(mlp->net[l].w, 1, sw, file)) ||
		    (sb != fread(mlp->net[l].b, 1, sb, file))) {
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
	int l, n, m;

	assert( mlp && pathname && strlen(pathname) );

	if (!(file = fopen(pathname, "wb"))) {
		RA_TRACE("unable to open file for writing");
		return -1;
	}
	for (l=1; l<mlp->layers; ++l) {
		n = size(mlp, l);
		m = size(mlp, l - 1);
		sw = (size_t)n * m * sizeof (mlp->net[0].w[0]);
		sb = (size_t)n * 1 * sizeof (mlp->net[0].b[0]);
		if ((sw != fwrite(mlp->net[l].w, 1, sw, file)) ||
		    (sb != fwrite(mlp->net[l].b, 1, sb, file))) {
			RA_TRACE("unable to write file");
			return -1;
		}
	}
	fclose(file);
	return 0;
}

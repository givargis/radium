/**
 * Copyright (c) Tony Givargis, 2020-2025
 *
 * ra_ec.h
 */

#ifndef _RA_EC_H_
#define _RA_EC_H_

#define RA__EC_MIN_K 1
#define RA__EC_MAX_K 255

#define RA__EC_MIN_N 8
#define RA__EC_MAX_N 262144 // 8^0, 8^1 ... 8^6

void ra__ec_init(void);

/*
 * [<------------------- buf ------------------->]
 *  [<--- n --->] [<--- n --->] ... [<--- n --->]
 *        0             1       ...       k
 */

void ra__ec_encode_pq(void *buf, int k, int n);

void ra__ec_encode_p(void *buf, int k, int n);

void ra__ec_encode_q(void *buf, int k, int n);

void ra__ec_encode_dp(void *buf, int k, int n, int x);

void ra__ec_encode_dq(void *buf, int k, int n, int x);

void ra__ec_encode_dd(void *buf, int k, int n, int x, int y);

int ra__ec_bist(void);

#endif // _RA_EC_H_

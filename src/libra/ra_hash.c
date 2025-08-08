//
// Copyright (c) Tony Givargis, 2024-2025
// givargis@uci.edu
// ra_hash.c
//

#include "ra_kernel.h"
#include "ra_hash.h"

#define MIX(a, b, c, d)                                                 \
        do {                                                            \
                (c) = (c) << 50 | (c) >> 14; (c) += (d); (a) ^= (c);    \
                (d) = (d) << 52 | (d) >> 12; (d) += (a); (b) ^= (d);    \
                (a) = (a) << 30 | (a) >> 34; (a) += (b); (c) ^= (a);    \
                (b) = (b) << 41 | (b) >> 23; (b) += (c); (d) ^= (b);    \
                (c) = (c) << 54 | (c) >> 10; (c) += (d); (a) ^= (c);    \
                (d) = (d) << 48 | (d) >> 16; (d) += (a); (b) ^= (d);    \
                (a) = (a) << 38 | (a) >> 26; (a) += (b); (c) ^= (a);    \
                (b) = (b) << 37 | (b) >> 27; (b) += (c); (d) ^= (b);    \
                (c) = (c) << 62 | (c) >>  2; (c) += (d); (a) ^= (c);    \
                (d) = (d) << 34 | (d) >> 30; (d) += (a); (b) ^= (d);    \
                (a) = (a) <<  5 | (a) >> 59; (a) += (b); (c) ^= (a);    \
                (b) = (b) << 36 | (b) >> 28; (b) += (c); (d) ^= (b);    \
        } while (0)

uint64_t
ra_hash(const void *buf, size_t len)
{
        const uint64_t MAGIC = 0xde79d3747be356ce;
        const char *p = (const char *)buf;
        uint64_t a, b, c, d;
        uint64_t u64[4];
        uint32_t u32[1];
        uint8_t u8[4];
        size_t q, r;

        assert( !len || buf );

        a = b = c = d = len + MAGIC;
        q = len / 32;
        r = len % 32;
        for (size_t i=0; i<q; ++i) {
                memcpy(&u64[0], p, sizeof (u64[0])); p += sizeof (u64[0]);
                memcpy(&u64[1], p, sizeof (u64[1])); p += sizeof (u64[1]);
                memcpy(&u64[2], p, sizeof (u64[2])); p += sizeof (u64[2]);
                memcpy(&u64[3], p, sizeof (u64[3])); p += sizeof (u64[3]);
                a += u64[0];
                b += u64[1];
                c += u64[2];
                d += u64[3];
                MIX(a, b, c, d);
        }
        q = r / 16;
        r = r % 16;
        for (size_t i=0; i<q; ++i) {
                memcpy(&u64[0], p, sizeof (u64[0])); p += sizeof (u64[0]);
                memcpy(&u64[1], p, sizeof (u64[1])); p += sizeof (u64[1]);
                a += u64[0];
                b += u64[1];
                MIX(a, b, c, d);
        }
        if (12 <= r) {
                memcpy(&u64[0], p, sizeof (u64[0])); p += sizeof (u64[0]);
                memcpy(&u32[0], p, sizeof (u32[0])); p += sizeof (u32[0]);
                memcpy(u8, p, r - 12);
                memset(u8 + r - 12, 0, 15 - r);
                c += u64[0];
                d += u32[0];
                d += (uint64_t)u8[0] << 32;
                d += (uint64_t)u8[1] << 40;
                d += (uint64_t)u8[2] << 48;
        }
        else if (8 <= r) {
                memcpy(&u64[0], p, sizeof (u64[0])); p += sizeof (u64[0]);
                memcpy(u8, p, r - 8);
                memset(u8 + r - 8, 0, 11 - r);
                c += u64[0];
                d += (uint64_t)u8[0];
                d += (uint64_t)u8[1] <<  8;
                d += (uint64_t)u8[2] << 16;
        }
        else if (4 <= r) {
                memcpy(&u32[0], p, sizeof (u32[0])); p += sizeof (u32[0]);
                memcpy(u8, p, r - 4);
                memset(u8 + r - 4, 0, 7 - r);
                c += u32[0];
                c += (uint64_t)u8[0] << 32;
                c += (uint64_t)u8[1] << 40;
                c += (uint64_t)u8[2] << 48;
        }
        else {
                memcpy(u8, p, r);
                memset(u8 + r, 0, 3 - r);
                c += (uint64_t)p[0];
                c += (uint64_t)p[1] <<  8;
                c += (uint64_t)p[2] << 16;
        }
        MIX(a, b, c, d);
        return a + b + c + d;
}

int
ra_hash_test(void)
{
        const int N = 123456;
        double stats[64];
        char buf[128];

        memset(stats, 0, sizeof (stats));
        for (int i=0; i<N; ++i) {
                for (uint64_t j=0; j<RA_ARRAY_SIZE(buf); ++j) {
                        buf[j] = (char)rand();
                }
                size_t len = rand() % (sizeof (buf));
                uint64_t hash = ra_hash(buf, len);
                for (int j=0; j<64; ++j) {
                        if (hash & (1LLU << j)) {
                                stats[j] += 1.0;
                        }
                }
        }
        for (int i=0; i<64; ++i) {
                stats[i] /= N;
                if ((0.55 < stats[i]) || (0.45 > stats[i])) {
                        RA_TRACE("software bug detected");
                        return -1;
                }
        }
        return 0;
}

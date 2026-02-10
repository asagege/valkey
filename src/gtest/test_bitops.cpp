/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <cstdint>
#include <cstdlib>
#include <time.h>

extern "C" {
#include "config.h"
#include "fmacros.h"
#include "zmalloc.h"

extern long long popcountScalar(void *s, long count);
#if HAVE_X86_SIMD
extern long long popcountAVX2(void *s, long count);
#endif
#if HAVE_ARM_NEON
extern long long popcountNEON(void *s, long count);
#endif
}

static long long bitcount(void *s, long count) {
    long long bits = 0;
    uint8_t *p = static_cast<uint8_t *>(s);
    for (int x = 0; x < count; x += 1) {
        uint8_t val = *(x + p);
        while (val) {
            bits += val & 1;
            val >>= 1;
        }
    }
    return bits;
}

static int test_case(const char *msg, int size) {
    UNUSED(msg);
    size_t bufsize = size > 0 ? size : 1;
    uint8_t *buf = static_cast<uint8_t *>(malloc(bufsize));
    if (!buf) return 1; /* Fatal - can't continue without buffer */

    int fuzzing = 1000;
    for (int y = 0; y < fuzzing; y += 1) {
        for (int z = 0; z < size; z += 1) {
            buf[z] = random() % 256;
        }

        long long expect = bitcount(buf, size);
        long long ret_scalar = popcountScalar(buf, size);
        if (expect != ret_scalar) {
            free(buf);
            return 1;
        }
#if HAVE_X86_SIMD
        long long ret_avx2 = popcountAVX2(buf, size);
        if (expect != ret_avx2) {
            free(buf);
            return 1;
        }
#endif
#if HAVE_ARM_NEON
        long long ret_neon = popcountNEON(buf, size);
        if (expect != ret_neon) {
            free(buf);
            return 1;
        }
#endif
    }

    free(buf);
    return 0;
}

/* Minimal test fixture */
class BitopsTest : public ::testing::Test {
};

TEST_F(BitopsTest, TestPopcount) {
#define TEST_CASE(MSG, SIZE)                                 \
    if (test_case("Test failed: " MSG, SIZE)) {              \
        FAIL() << "Test failed: " MSG;                       \
    }

    /* The AVX2 version divides the array into the following 3 parts.
     *        Part A         Part B       Part C
     * +-----------------+--------------+---------+
     * | 8 * 32bytes * X |  32bytes * Y | Z bytes |
     * +-----------------+--------------+---------+
     */
    /* So we test the following cases */
    TEST_CASE("Popcount(Part A)", 8 * 32 * 2);
    TEST_CASE("Popcount(Part B)", 32 * 2);
    TEST_CASE("Popcount(Part C)", 2);
    TEST_CASE("Popcount(Part A + Part B)", 8 * 32 * 7 + 32 * 2);
    TEST_CASE("Popcount(Part A + Part C)", 8 * 32 * 11 + 7);
    TEST_CASE("Popcount(Part A + Part B + Part C)", 8 * 32 * 3 + 3 * 32 + 5);
    TEST_CASE("Popcount(Corner case)", 0);
#undef TEST_CASE
}

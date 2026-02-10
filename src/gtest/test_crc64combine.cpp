/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>

extern "C" {
#include "crc64.h"
#include "crccombine.h"
#include "crcspeed.h"
#include "fmacros.h"
#include "zmalloc.h"
}

static long long _ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, nullptr);
    ust = static_cast<long long>(tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

static void genBenchmarkRandomData(char *data, int count) {
    static uint32_t state = 1234;
    int i = 0;

    while (count--) {
        state = (state * 1103515245 + 12345);
        data[i++] = '0' + ((state >> 16) & 63);
    }
}

static int bench_crc64(unsigned char *data, uint64_t size, long long passes, uint64_t check, const char *name, int csv) {
    uint64_t min = size, hash = 0;
    long long original_start = _ustime(), original_end;
    for (long long i = passes; i > 0; i--) {
        hash = crc64(0, data, size);
    }
    original_end = _ustime();
    min = (original_end - original_start) * 1000 / passes;
    /* approximate nanoseconds without nstime */
    if (csv) {
        printf("%s,%" PRIu64 ",%" PRIu64 ",%d\n", name, size, (1000 * size) / min, hash == check);
    } else {
        printf("test size=%" PRIu64 " algorithm=%s %" PRIu64 " M/sec matches=%d\n", size, name,
               (1000 * size) / min, hash == check);
    }
    return hash != check;
}

const uint64_t BENCH_RPOLY = UINT64_C(0x95ac9329ac4bc9b5);

static void bench_combine(const char *label, uint64_t size, uint64_t expect, int csv) {
    uint64_t min = size, start = expect, thash = expect ^ (expect >> 17);
    long long original_start = _ustime(), original_end;
    for (int i = 0; i < 1000; i++) {
        crc64_combine(thash, start, size, BENCH_RPOLY, 64);
    }
    original_end = _ustime();
    /* ran 1000 times, want ns per, counted us per 1000 ... */
    min = original_end - original_start;
    if (csv) {
        printf("%s,%" PRIu64 ",%" PRIu64 "\n", label, size, min);
    } else {
        printf("%s size=%" PRIu64 " in %" PRIu64 " nsec\n", label, size, min);
    }
}

class Crc64CombineTest : public ::testing::Test {};

/* This is a special unit test useful for benchmarking crc64combine performance.
 * To run this test explicitly, use:
 *   ./src/gtest/valkey-unit-gtests --gtest_filter=Crc64CombineTest.DISABLED_TestCrc64CombineBenchmark --gtest_also_run_disabled_tests
 *
 * The original C version supported command-line arguments:
 *   --csv              Output in CSV format
 *   -l                 Loop. Run the tests forever
 *   --crc <bytes>      Benchmark crc64 faster options, using a buffer this big
 *   --combine          Benchmark crc64 combine value ranges and timings
 *
 * For GoogleTest, use --gtest_repeat=-1 for infinite looping.
 * Modify the test code below to change buffer size, csv output, or combine mode. */
TEST_F(Crc64CombineTest, DISABLED_TestCrc64CombineBenchmark) {
    uint64_t crc64_test_size = 16384; // Default size (change as needed)
    int csv = 0, combine = 0;

    // Note: The original C version had a 'loop' variable for infinite benchmarking.
    // In GoogleTest, use --gtest_repeat=-1 to run tests infinitely instead.

    // The original C version also had a do-while loop that stepped down test sizes.
    // For simplicity, this GoogleTest version runs a single size. To test multiple
    // sizes, modify crc64_test_size above or run the test multiple times.

    do {
        unsigned char *data = nullptr;
        uint64_t passes = 0;
        if (crc64_test_size) {
            data = static_cast<unsigned char *>(zmalloc(crc64_test_size));
            genBenchmarkRandomData(reinterpret_cast<char *>(data), crc64_test_size);
            /* We want to hash about 1 gig of data in total, looped, to get a good
             * idea of our performance. */
            passes = (UINT64_C(0x100000000) / crc64_test_size);
            passes = passes >= 2 ? passes : 2;
            passes = passes <= 1000 ? passes : 1000;
        }

        crc64_init();
        /* warm up the cache */
        set_crc64_cutoffs(crc64_test_size + 1, crc64_test_size + 1);
        uint64_t expect = crc64(0, data, crc64_test_size);

        if (!combine && crc64_test_size) {
            if (csv) printf("algorithm,buffer,performance,crc64_matches\n");

            /* get the single-character version for single-byte Redis behavior */
            set_crc64_cutoffs(0, crc64_test_size + 1);
            EXPECT_EQ(bench_crc64(data, crc64_test_size, passes, expect, "crc_1byte", csv), 0);

            set_crc64_cutoffs(crc64_test_size + 1, crc64_test_size + 1);
            /* run with 8-byte "single" path, crcfaster */
            EXPECT_EQ(bench_crc64(data, crc64_test_size, passes, expect, "crcspeed", csv), 0);

            /* run with dual 8-byte paths */
            set_crc64_cutoffs(1, crc64_test_size + 1);
            EXPECT_EQ(bench_crc64(data, crc64_test_size, passes, expect, "crcdual", csv), 0);

            /* run with tri 8-byte paths */
            set_crc64_cutoffs(1, 1);
            EXPECT_EQ(bench_crc64(data, crc64_test_size, passes, expect, "crctri", csv), 0);
        }

        uint64_t INIT_SIZE = UINT64_C(0xffffffffffffffff);
        if (combine) {
            long long init_start = _ustime();
            crc64_combine(UINT64_C(0xdeadbeefdeadbeef), UINT64_C(0xfeebdaedfeebdaed), INIT_SIZE, BENCH_RPOLY, 64);
            long long init_end = _ustime();

            init_end -= init_start;
            init_end *= 1000;
            if (csv) {
                printf("operation,size,nanoseconds\n");
                printf("init_64,%" PRIu64 ",%" PRIu64 "\n", INIT_SIZE, static_cast<uint64_t>(init_end));
            } else {
                printf("init_64 size=%" PRIu64 " in %" PRIu64 " nsec\n", INIT_SIZE, static_cast<uint64_t>(init_end));
            }
            /* use the hash itself as the size (unpredictable) */
            bench_combine("hash_as_size_combine", crc64_test_size, expect, csv);
            /* let's do something big (predictable, so fast) */
            bench_combine("largest_combine", INIT_SIZE, expect, csv);
            bench_combine("combine", crc64_test_size, expect, csv);
        }

        /* Be free memory region, be free. */
        if (data) zfree(data);

        /* step down by ~1.641 for a range of test sizes */
        crc64_test_size -= (crc64_test_size >> 2) + (crc64_test_size >> 3) + (crc64_test_size >> 6);
    } while (crc64_test_size > 3);
}

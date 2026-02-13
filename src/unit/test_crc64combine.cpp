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
    ust = (long long)(tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

static int bench_crc64(unsigned char *data, uint64_t size, long long passes, uint64_t check, char *name, int csv) {
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

static void bench_combine(char *label, uint64_t size, uint64_t expect, int csv) {
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

static void genBenchmarkRandomData(char *data, int count) {
    static uint32_t state = 1234;
    int i = 0;

    while (count--) {
        state = (state * 1103515245 + 12345);
        data[i++] = '0' + ((state >> 16) & 63);
    }
}

/* Parameter structure for CRC64 benchmark tests */
struct BenchmarkParams {
    uint64_t buffer_size;
    bool csv_output;
    bool combine_mode;

    std::string ToString() const {
        std::string result = std::to_string(buffer_size);
        if (csv_output) result += "_csv";
        if (combine_mode) result += "_combine";
        if (!csv_output && !combine_mode) result += "_crc";
        return result;
    }
};

/* Utility function to calculate the number of benchmark passes */
static uint64_t CalculatePasses(uint64_t buffer_size) {
    uint64_t passes = (UINT64_C(0x100000000) / buffer_size);
    passes = passes >= 2 ? passes : 2;
    passes = passes <= 1000 ? passes : 1000;
    return passes;
}

/* Parameterized test class for CRC64 benchmarks */
class Crc64CombineTest : public ::testing::TestWithParam<BenchmarkParams> {
  protected:
    void RunCrcBenchmarks(unsigned char *data, uint64_t size, uint64_t passes, uint64_t expect) {
        auto params = GetParam();

        if (params.csv_output) {
            printf("algorithm,buffer,performance,crc64_matches\n");
        }

        /* get the single-character version for single-byte Redis behavior */
        set_crc64_cutoffs(0, size + 1);
        ASSERT_EQ(bench_crc64(data, size, passes, expect, (char *)"crc_1byte", params.csv_output), 0);

        set_crc64_cutoffs(size + 1, size + 1);
        /* run with 8-byte "single" path, crcfaster */
        ASSERT_EQ(bench_crc64(data, size, passes, expect, (char *)"crcspeed", params.csv_output), 0);

        /* run with dual 8-byte paths */
        set_crc64_cutoffs(1, size + 1);
        ASSERT_EQ(bench_crc64(data, size, passes, expect, (char *)"crcdual", params.csv_output), 0);

        /* run with tri 8-byte paths */
        set_crc64_cutoffs(1, 1);
        ASSERT_EQ(bench_crc64(data, size, passes, expect, (char *)"crctri", params.csv_output), 0);
    }

    void RunCombineBenchmarks(uint64_t size, uint64_t expect) {
        auto params = GetParam();
        const uint64_t INIT_SIZE = UINT64_C(0xffffffffffffffff);

        long long init_start = _ustime();
        crc64_combine(UINT64_C(0xdeadbeefdeadbeef), UINT64_C(0xfeebdaedfeebdaed), INIT_SIZE, BENCH_RPOLY, 64);
        long long init_end = _ustime();

        init_end = (init_end - init_start) * 1000;
        if (params.csv_output) {
            printf("operation,size,nanoseconds\n");
            printf("init_64,%" PRIu64 ",%" PRIu64 "\n", INIT_SIZE, (uint64_t)(init_end));
        } else {
            printf("init_64 size=%" PRIu64 " in %" PRIu64 " nsec\n", INIT_SIZE, (uint64_t)(init_end));
        }
        /* use the hash itself as the size (unpredictable) */
        bench_combine((char *)"hash_as_size_combine", size, expect, params.csv_output);
        /* let's do something big (predictable, so fast) */
        bench_combine((char *)"largest_combine", INIT_SIZE, expect, params.csv_output);
        bench_combine((char *)"combine", size, expect, params.csv_output);
    }
};

/* This is a special unit test useful for benchmarking crc64combine performance.
 *
 * To run all benchmark tests:
 *   ./src/unit/valkey-unit-gtests --gtest_filter='*Crc64CombineTest*' --gtest_also_run_disabled_tests
 *
 * To run specific buffer size (e.g., 16384 bytes):
 *   ./src/unit/valkey-unit-gtests --gtest_filter='*Crc64CombineTest*16384*' --gtest_also_run_disabled_tests
 *
 * To run CSV output tests:
 *   ./src/unit/valkey-unit-gtests --gtest_filter='*Crc64CombineTest*csv*' --gtest_also_run_disabled_tests
 *
 * To run combine mode tests:
 *   ./src/unit/valkey-unit-gtests --gtest_filter='*Crc64CombineTest*combine*' --gtest_also_run_disabled_tests
 *
 * For infinite looping (equivalent to original -l flag):
 *   ./src/unit/valkey-unit-gtests --gtest_filter='*Crc64CombineTest*' --gtest_also_run_disabled_tests --gtest_repeat=-1
 *
 * Migration notes from original C test:
 *   - Command-line args replaced with test parameters and GoogleTest filters
 *   - Loop mode (-l) replaced with --gtest_repeat=-1
 *   - Step-down buffer sizes replaced with explicit parameter values
 *   - Use --gtest_filter to select specific configurations
 *   - Note: In zsh/bash, quote the filter pattern to prevent shell glob expansion
 */
TEST_P(Crc64CombineTest, DISABLED_BenchmarkCrc64) {
    auto params = GetParam();

    /* Allocate and initialize test data */
    unsigned char *data = nullptr;
    uint64_t passes = 0;
    if (params.buffer_size) {
        data = (unsigned char *)(zmalloc(params.buffer_size));
        genBenchmarkRandomData((char *)(data), params.buffer_size);
        /* We want to hash about 1 gig of data in total, looped, to get a good
         * idea of our performance. */
        passes = CalculatePasses(params.buffer_size);
    }

    crc64_init();
    /* warm up the cache */
    set_crc64_cutoffs(params.buffer_size + 1, params.buffer_size + 1);
    uint64_t expect = crc64(0, data, params.buffer_size);

    if (params.combine_mode) {
        RunCombineBenchmarks(params.buffer_size, expect);
    } else if (params.buffer_size) {
        RunCrcBenchmarks(data, params.buffer_size, passes, expect);
    }

    /* Be free memory region, be free. */
    if (data) zfree(data);
}

/* Define test parameters for different benchmark configurations */
INSTANTIATE_TEST_SUITE_P(
    BufferSizes,
    Crc64CombineTest,
    ::testing::Values(
        BenchmarkParams{1024, false, false},
        BenchmarkParams{4096, false, false},
        BenchmarkParams{16384, false, false},
        BenchmarkParams{65536, false, false},
        BenchmarkParams{16384, true, false}, // CSV output
        BenchmarkParams{16384, false, true}  // Combine mode
        ),
    [](const ::testing::TestParamInfo<BenchmarkParams> &info) { return info.param.ToString(); });

/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <strings.h>
#include <cstring>
#include <cstdio>

extern "C" {
#include "fmacros.h"
#include "util.h"
#include "mt19937-64.h"
#include "hashtable.h"
#include "zmalloc.h"

/* We override the default assertion mechanism, so that it prints out info and then dies. */
void _serverAssert(const char *estr, const char *file, int line) {
    fprintf(stderr, "[serverAssert - %s:%d] - %s\n", file, line, estr);
    exit(1);
}
}

/* Custom test event listener to track memory usage */
class MemoryLeakListener : public ::testing::EmptyTestEventListener {
private:
    size_t used_mem_before;

public:
    void OnTestStart(const ::testing::TestInfo& test_info) override {
        (void)test_info; // Mark as intentionally unused
        used_mem_before = zmalloc_used_memory();
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        size_t used_mem_after = zmalloc_used_memory();
        if (used_mem_after != used_mem_before) {
            long long leak = static_cast<long long>(used_mem_after) - static_cast<long long>(used_mem_before);
            fprintf(stderr, "[MEMORY LEAK] %s.%s: %lld bytes leaked\n",
                    test_info.test_suite_name(), test_info.name(), leak);
            // Optionally fail the test on memory leak
            // ADD_FAILURE() << "Memory leak detected: " << leak << " bytes";
        }
    }
};

int valkey_test_main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Parse custom flags
    char *seed = nullptr;

    for (int j = 1; j < argc; j++) {
        char *arg = argv[j];
        if (!strcasecmp(arg, "--seed") && (j + 1 < argc)) {
            seed = argv[j + 1];
            j++; // Skip the next argument
        }
    }

    // Initialize random seed
    if (seed) {
        setRandomSeedCString(seed, strlen(seed));
    }

    char seed_cstr[129];
    getRandomSeedCString(seed_cstr, 129);
    printf("Tests will run with seed=%s\n", seed_cstr);

    // Initialize random number generators
    unsigned long long genrandseed;
    getRandomBytes(reinterpret_cast<unsigned char *>(&genrandseed), sizeof(genrandseed));

    uint8_t hashseed[16];
    getRandomBytes(hashseed, sizeof(hashseed));

    // Set seeds for reproducibility
    setRandomSeedCString(seed_cstr, strlen(seed_cstr));
    init_genrand64(genrandseed);
    srandom(static_cast<unsigned>(genrandseed));
    hashtableSetHashFunctionSeed(hashseed);

    // Add memory leak listener
    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new MemoryLeakListener());

    // Run all tests
    int result = RUN_ALL_TESTS();

    return result;
}

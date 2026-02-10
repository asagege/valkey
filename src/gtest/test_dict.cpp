/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <sys/time.h>

extern "C" {
#include "dict.h"

/* Wrapper function declarations for accessing static dict.c internals */
unsigned int gtest_dict_get_force_resize_ratio(void);
signed char gtest_dict_next_exp(unsigned long size);
long long gtest_time_in_milliseconds(void);
}

uint64_t hashCallback(const void *key) {
    return dictGenHashFunction(static_cast<const unsigned char *>(key), strlen(static_cast<const char *>(key)));
}

int compareCallback(const void *key1, const void *key2) {
    int l1, l2;
    l1 = strlen(static_cast<const char *>(key1));
    l2 = strlen(static_cast<const char *>(key2));
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

void freeCallback(void *val) {
    zfree(val);
}

char *stringFromLongLong(long long value) {
    char buf[32];
    int len;
    char *s;

    len = snprintf(buf, sizeof(buf), "%lld", value);
    s = static_cast<char *>(zmalloc(len + 1));
    memcpy(s, buf, len);
    s[len] = '\0';
    return s;
}

dictType BenchmarkDictType = {hashCallback, nullptr, compareCallback, freeCallback, nullptr, nullptr};

class DictTest : public ::testing::Test {
protected:
    dict *_dict = nullptr;
    long j;
    int retval;
    unsigned long new_dict_size, current_dict_used, remain_keys;

    static void SetUpTestSuite() {
        monotonicInit(); /* Required for dict tests, that are relying on monotime during dict rehashing. */
    }

    void SetUp() override {
        _dict = dictCreate(&BenchmarkDictType);
    }

    void TearDown() override {
        if (_dict) {
            dictRelease(_dict);
            _dict = nullptr;
        }
    }
};

TEST_F(DictTest, dictAdd16Keys) {
    /* Add 16 keys and verify dict resize is ok */
    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    for (j = 0; j < 16; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);
    EXPECT_EQ(dictSize(_dict), 16u);
    EXPECT_EQ(dictBuckets(_dict), 16u);
}

TEST_F(DictTest, dictDisableResize) {
    /* Use DICT_RESIZE_AVOID to disable the dict resize and pad to (dict_force_resize_ratio * 16) */

    /* First add 16 keys */
    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    for (j = 0; j < 16; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    /* Use DICT_RESIZE_AVOID to disable the dict resize, and pad
     * the number of keys to (dict_force_resize_ratio * 16), so we can satisfy
     * dict_force_resize_ratio in next test. */
    dictSetResizeEnabled(DICT_RESIZE_AVOID);
    for (j = 16; j < static_cast<long>(gtest_dict_get_force_resize_ratio() * 16); j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = gtest_dict_get_force_resize_ratio() * 16;
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(dictBuckets(_dict), 16u);
}

TEST_F(DictTest, dictAddOneKeyTriggerResize) {
    /* Add one more key, trigger the dict resize */

    /* Setup: Add keys up to dict_force_resize_ratio * 16 */
    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    for (j = 0; j < 16; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    dictSetResizeEnabled(DICT_RESIZE_AVOID);
    for (j = 16; j < static_cast<long>(gtest_dict_get_force_resize_ratio() * 16); j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = gtest_dict_get_force_resize_ratio() * 16;

    retval = dictAdd(_dict, stringFromLongLong(current_dict_used), reinterpret_cast<void *>(current_dict_used));
    EXPECT_EQ(retval, DICT_OK);
    current_dict_used++;
    new_dict_size = 1UL << gtest_dict_next_exp(current_dict_used);
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[0]), 16u);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[1]), new_dict_size);

    /* Wait for rehashing. */
    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[0]), new_dict_size);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[1]), 0u);
}

TEST_F(DictTest, dictDeleteKeys) {
    /* Delete keys until we can trigger shrink in next test */

    /* Setup: Add keys and trigger resize */
    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    for (j = 0; j < 16; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    dictSetResizeEnabled(DICT_RESIZE_AVOID);
    for (j = 16; j < static_cast<long>(gtest_dict_get_force_resize_ratio() * 16); j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = gtest_dict_get_force_resize_ratio() * 16;

    retval = dictAdd(_dict, stringFromLongLong(current_dict_used), reinterpret_cast<void *>(current_dict_used));
    EXPECT_EQ(retval, DICT_OK);
    current_dict_used++;
    new_dict_size = 1UL << gtest_dict_next_exp(current_dict_used);

    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    /* Delete keys until we can satisfy (1 / HASHTABLE_MIN_FILL) in the next test. */
    for (j = new_dict_size / HASHTABLE_MIN_FILL + 1; j < static_cast<long>(current_dict_used); j++) {
        char *key = stringFromLongLong(j);
        retval = dictDelete(_dict, key);
        zfree(key);
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = new_dict_size / HASHTABLE_MIN_FILL + 1;
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[0]), new_dict_size);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[1]), 0u);
}

TEST_F(DictTest, dictDeleteOneKeyTriggerResize) {
    /* Delete one more key, trigger the dict resize */

    /* Setup: Add keys, trigger resize, then delete keys */
    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    for (j = 0; j < 16; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    dictSetResizeEnabled(DICT_RESIZE_AVOID);
    for (j = 16; j < static_cast<long>(gtest_dict_get_force_resize_ratio() * 16); j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = gtest_dict_get_force_resize_ratio() * 16;

    retval = dictAdd(_dict, stringFromLongLong(current_dict_used), reinterpret_cast<void *>(current_dict_used));
    EXPECT_EQ(retval, DICT_OK);
    current_dict_used++;
    new_dict_size = 1UL << gtest_dict_next_exp(current_dict_used);

    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    for (j = new_dict_size / HASHTABLE_MIN_FILL + 1; j < static_cast<long>(current_dict_used); j++) {
        char *key = stringFromLongLong(j);
        retval = dictDelete(_dict, key);
        zfree(key);
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = new_dict_size / HASHTABLE_MIN_FILL + 1;

    current_dict_used--;
    char *key = stringFromLongLong(current_dict_used);
    retval = dictDelete(_dict, key);
    zfree(key);
    unsigned long oldDictSize = new_dict_size;
    new_dict_size = 1UL << gtest_dict_next_exp(current_dict_used);
    EXPECT_EQ(retval, DICT_OK);
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[0]), oldDictSize);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[1]), new_dict_size);

    /* Wait for rehashing. */
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[0]), new_dict_size);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[1]), 0u);
}

TEST_F(DictTest, dictEmptyDirAdd128Keys) {
    /* Empty the dictionary and add 128 keys */
    dictEmpty(_dict, nullptr);
    for (j = 0; j < 128; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);
    EXPECT_EQ(dictSize(_dict), 128u);
    EXPECT_EQ(dictBuckets(_dict), 128u);
}

TEST_F(DictTest, dictDisableResizeReduceTo3) {
    /* Use DICT_RESIZE_AVOID to disable the dict resize and reduce to 3 */

    /* Setup: Add 128 keys */
    dictEmpty(_dict, nullptr);
    for (j = 0; j < 128; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    /* Use DICT_RESIZE_AVOID to disable the dict reset, and reduce
     * the number of keys until we can trigger shrinking in next test. */
    dictSetResizeEnabled(DICT_RESIZE_AVOID);
    remain_keys = DICTHT_SIZE(_dict->ht_size_exp[0]) / (HASHTABLE_MIN_FILL * gtest_dict_get_force_resize_ratio()) + 1;
    for (j = remain_keys; j < 128; j++) {
        char *key = stringFromLongLong(j);
        retval = dictDelete(_dict, key);
        zfree(key);
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = remain_keys;
    EXPECT_EQ(dictSize(_dict), remain_keys);
    EXPECT_EQ(dictBuckets(_dict), 128u);
}

TEST_F(DictTest, dictDeleteOneKeyTriggerResizeAgain) {
    /* Delete one more key, trigger the dict resize */

    /* Setup: Add 128 keys and reduce */
    dictEmpty(_dict, nullptr);
    dictSetResizeEnabled(DICT_RESIZE_ENABLE); /* Reset resize mode before adding keys */
    for (j = 0; j < 128; j++) {
        retval = dictAdd(_dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);

    dictSetResizeEnabled(DICT_RESIZE_AVOID);
    remain_keys = DICTHT_SIZE(_dict->ht_size_exp[0]) / (HASHTABLE_MIN_FILL * gtest_dict_get_force_resize_ratio()) + 1;
    for (j = remain_keys; j < 128; j++) {
        char *key = stringFromLongLong(j);
        retval = dictDelete(_dict, key);
        zfree(key);
        EXPECT_EQ(retval, DICT_OK);
    }
    current_dict_used = remain_keys;

    current_dict_used--;
    char *key = stringFromLongLong(current_dict_used);
    retval = dictDelete(_dict, key);
    zfree(key);
    new_dict_size = 1UL << gtest_dict_next_exp(current_dict_used);
    EXPECT_EQ(retval, DICT_OK);
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[0]), 128u);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[1]), new_dict_size);

    /* Wait for rehashing. */
    dictSetResizeEnabled(DICT_RESIZE_ENABLE);
    while (dictIsRehashing(_dict)) dictRehashMicroseconds(_dict, 1000);
    EXPECT_EQ(dictSize(_dict), current_dict_used);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[0]), new_dict_size);
    EXPECT_EQ(DICTHT_SIZE(_dict->ht_size_exp[1]), 0u);
}

/* This is a benchmark test for dict performance.
 * To run this test explicitly, use:
 *   ./src/gtest/valkey-unit-gtests --gtest_filter=DictTest.DISABLED_dictBenchmark --gtest_also_run_disabled_tests
 */
TEST_F(DictTest, DISABLED_dictBenchmark) {
    long j;
    long long start, elapsed;
    int retval;
    dict *dict = dictCreate(&BenchmarkDictType);
    long count = 5000;

    #define start_benchmark() start = gtest_time_in_milliseconds()
    #define end_benchmark(msg)                                      \
        do {                                                        \
            elapsed = gtest_time_in_milliseconds() - start;         \
            printf(msg ": %ld items in %lld ms\n", count, elapsed); \
        } while (0)

    start_benchmark();
    for (j = 0; j < count; j++) {
        retval = dictAdd(dict, stringFromLongLong(j), reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    end_benchmark("Inserting");
    EXPECT_EQ(static_cast<long>(dictSize(dict)), count);

    /* Wait for rehashing. */
    while (dictIsRehashing(dict)) {
        dictRehashMicroseconds(dict, 100 * 1000);
    }

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictFind(dict, key);
        EXPECT_NE(de, nullptr);
        zfree(key);
    }
    end_benchmark("Linear access of existing elements");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictFind(dict, key);
        EXPECT_NE(de, nullptr);
        zfree(key);
    }
    end_benchmark("Linear access of existing elements (2nd round)");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(rand() % count);
        dictEntry *de = dictFind(dict, key);
        EXPECT_NE(de, nullptr);
        zfree(key);
    }
    end_benchmark("Random access of existing elements");

    start_benchmark();
    for (j = 0; j < count; j++) {
        dictEntry *de = dictGetRandomKey(dict);
        EXPECT_NE(de, nullptr);
    }
    end_benchmark("Accessing random keys");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(rand() % count);
        key[0] = 'X';
        dictEntry *de = dictFind(dict, key);
        EXPECT_EQ(de, nullptr);
        zfree(key);
    }
    end_benchmark("Accessing missing");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        retval = dictDelete(dict, key);
        EXPECT_EQ(retval, DICT_OK);
        key[0] += 17; /* Change first number to letter. */
        retval = dictAdd(dict, key, reinterpret_cast<void *>(j));
        EXPECT_EQ(retval, DICT_OK);
    }
    end_benchmark("Removing and adding");
    dictRelease(dict);

    #undef start_benchmark
    #undef end_benchmark
}

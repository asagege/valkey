/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <cstdlib>
#include <ctime>
#include <sys/time.h>

extern "C" {
#include "intset.h"

/* Wrapper function declarations for accessing static intset.c internals */
uint8_t gtest_intset_value_encoding(int64_t v);
int64_t gtest_intset_get_encoded(intset *is, int pos, uint8_t enc);
int64_t gtest_intset_get(intset *is, int pos);
uint8_t gtest_intset_search(intset *is, int64_t value, uint32_t *pos);
}

/* Macros from intset.c needed for testing */
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

static long long usec(void) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (static_cast<long long>(tv.tv_sec) * 1000000) + tv.tv_usec;
}

static intset *createSet(int bits, int size) {
    uint64_t mask = (1 << bits) - 1;
    uint64_t value;
    intset *is = intsetNew();

    for (int i = 0; i < size; i++) {
        if (bits > 32) {
            value = (rand() * rand()) & mask;
        } else {
            value = rand() & mask;
        }
        is = intsetAdd(is, value, nullptr);
    }
    return is;
}

static int checkConsistency(intset *is) {
    for (uint32_t i = 0; i < (intrev32ifbe(is->length) - 1); i++) {
        uint32_t encoding = intrev32ifbe(is->encoding);

        if (encoding == INTSET_ENC_INT16) {
            int16_t *i16 = reinterpret_cast<int16_t *>(is->contents);
            EXPECT_LT(i16[i], i16[i + 1]);
        } else if (encoding == INTSET_ENC_INT32) {
            int32_t *i32 = reinterpret_cast<int32_t *>(is->contents);
            EXPECT_LT(i32[i], i32[i + 1]);
        } else {
            int64_t *i64 = reinterpret_cast<int64_t *>(is->contents);
            EXPECT_LT(i64[i], i64[i + 1]);
        }
    }
    return 1;
}

class IntsetTest : public ::testing::Test {};

TEST_F(IntsetTest, TestIntsetValueEncodings) {
    EXPECT_EQ(gtest_intset_value_encoding(-32768), INTSET_ENC_INT16);
    EXPECT_EQ(gtest_intset_value_encoding(+32767), INTSET_ENC_INT16);
    EXPECT_EQ(gtest_intset_value_encoding(-32769), INTSET_ENC_INT32);
    EXPECT_EQ(gtest_intset_value_encoding(+32768), INTSET_ENC_INT32);
    EXPECT_EQ(gtest_intset_value_encoding(-2147483648), INTSET_ENC_INT32);
    EXPECT_EQ(gtest_intset_value_encoding(+2147483647), INTSET_ENC_INT32);
    EXPECT_EQ(gtest_intset_value_encoding(-2147483649), INTSET_ENC_INT64);
    EXPECT_EQ(gtest_intset_value_encoding(+2147483648), INTSET_ENC_INT64);
    EXPECT_EQ(gtest_intset_value_encoding(-9223372036854775808ull), INTSET_ENC_INT64);
    EXPECT_EQ(gtest_intset_value_encoding(+9223372036854775807ull), INTSET_ENC_INT64);
}

TEST_F(IntsetTest, TestIntsetBasicAdding) {
    intset *is = intsetNew();
    uint8_t success;
    is = intsetAdd(is, 5, &success);
    EXPECT_TRUE(success);
    is = intsetAdd(is, 6, &success);
    EXPECT_TRUE(success);
    is = intsetAdd(is, 4, &success);
    EXPECT_TRUE(success);
    is = intsetAdd(is, 4, &success);
    EXPECT_FALSE(success);
    EXPECT_EQ(intsetMax(is), 6);
    EXPECT_EQ(intsetMin(is), 4);
    zfree(is);
}

TEST_F(IntsetTest, TestIntsetLargeNumberRandomAdd) {
    uint32_t inserts = 0;
    uint8_t success;
    intset *is = intsetNew();
    for (int i = 0; i < 1024; i++) {
        is = intsetAdd(is, rand() % 0x800, &success);
        if (success) inserts++;
    }
    EXPECT_EQ(intrev32ifbe(is->length), inserts);
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);
}

TEST_F(IntsetTest, TestIntsetUpgradeFromint16Toint32) {
    intset *is = intsetNew();
    is = intsetAdd(is, 32, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT16);
    is = intsetAdd(is, 65535, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT32);
    EXPECT_TRUE(intsetFind(is, 32));
    EXPECT_TRUE(intsetFind(is, 65535));
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);

    is = intsetNew();
    is = intsetAdd(is, 32, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT16);
    is = intsetAdd(is, -65535, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT32);
    EXPECT_TRUE(intsetFind(is, 32));
    EXPECT_TRUE(intsetFind(is, -65535));
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);
}

TEST_F(IntsetTest, TestIntsetUpgradeFromint16Toint64) {
    intset *is = intsetNew();
    is = intsetAdd(is, 32, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT16);
    is = intsetAdd(is, 4294967295, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT64);
    EXPECT_TRUE(intsetFind(is, 32));
    EXPECT_TRUE(intsetFind(is, 4294967295));
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);

    is = intsetNew();
    is = intsetAdd(is, 32, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT16);
    is = intsetAdd(is, -4294967295, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT64);
    EXPECT_TRUE(intsetFind(is, 32));
    EXPECT_TRUE(intsetFind(is, -4294967295));
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);
}

TEST_F(IntsetTest, TestIntsetUpgradeFromint32Toint64) {
    intset *is = intsetNew();
    is = intsetAdd(is, 65535, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT32);
    is = intsetAdd(is, 4294967295, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT64);
    EXPECT_TRUE(intsetFind(is, 65535));
    EXPECT_TRUE(intsetFind(is, 4294967295));
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);

    is = intsetNew();
    is = intsetAdd(is, 65535, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT32);
    is = intsetAdd(is, -4294967295, nullptr);
    EXPECT_EQ(intrev32ifbe(is->encoding), INTSET_ENC_INT64);
    EXPECT_TRUE(intsetFind(is, 65535));
    EXPECT_TRUE(intsetFind(is, -4294967295));
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);
}

TEST_F(IntsetTest, TestIntsetStressLookups) {
    long num = 100000, size = 10000;
    int i, bits = 20;
    long long start;
    intset *is = createSet(bits, size);
    EXPECT_EQ(checkConsistency(is), 1);

    start = usec();
    for (i = 0; i < num; i++) gtest_intset_search(is, rand() % ((1 << bits) - 1), nullptr);
    printf("%ld lookups, %ld element set, %lldusec\n", num, size, usec() - start);
    zfree(is);
}

TEST_F(IntsetTest, TestIntsetStressAddDelete) {
    int i, v1, v2;
    intset *is = intsetNew();
    for (i = 0; i < 0xffff; i++) {
        v1 = rand() % 0xfff;
        is = intsetAdd(is, v1, nullptr);
        EXPECT_TRUE(intsetFind(is, v1));

        v2 = rand() % 0xfff;
        is = intsetRemove(is, v2, nullptr);
        EXPECT_FALSE(intsetFind(is, v2));
    }
    EXPECT_EQ(checkConsistency(is), 1);
    zfree(is);
}

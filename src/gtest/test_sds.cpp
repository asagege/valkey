/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <climits>
#include <cstdio>
#include <cstring>

extern "C" {
#include "sds.h"
#include "sdsalloc.h"
#include "util.h"
}

static sds sdsTestTemplateCallback(const_sds varname, void *arg) {
    UNUSED(arg);
    static const char *_var1 = "variable1";
    static const char *_var2 = "variable2";

    if (!strcmp(varname, _var1))
        return sdsnew("value1");
    else if (!strcmp(varname, _var2))
        return sdsnew("value2");
    else
        return NULL;
}

class SdsTest : public ::testing::Test {}; // Empty fixture for test organization and filtering

TEST_F(SdsTest, TestSds) {
    sds x = sdsnew("foo"), y;

    /* Create a string and obtain the length */
    EXPECT_STREQ(x, "foo");

    sdsfree(x);
    x = sdsnewlen("foo", 2);
    /* Create a string with specified length */
    EXPECT_STREQ(x, "fo");

    x = sdscat(x, "bar");
    /* Strings concatenation */
    EXPECT_STREQ(x, "fobar");

    x = sdscpy(x, "a");
    /* sdscpy() against an originally longer string */
    EXPECT_STREQ(x, "a");

    x = sdscpy(x, "xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk");
    /* sdscpy() against an originally shorter string */
    EXPECT_STREQ(x, "xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk");

    sdsfree(x);
    x = sdscatprintf(sdsempty(), "%d", 123);
    /* sdscatprintf() seems working in the base case */
    EXPECT_STREQ(x, "123");

    sdsfree(x);
    x = sdscatprintf(sdsempty(), "a%cb", 0);
    /* sdscatprintf() seems working with \0 inside of result */
    EXPECT_EQ(sdslen(x), 3);
    EXPECT_EQ(memcmp(x, "a\0b\0", 4), 0);

    sdsfree(x);
    size_t etalon_size = 1024 * 1024;
    char *etalon = static_cast<char *>(malloc(etalon_size));
    for (size_t i = 0; i < etalon_size; i++) {
        etalon[i] = '0';
    }
    x = sdscatprintf(sdsempty(), "%0*d", static_cast<int>(etalon_size), 0);
    /* sdscatprintf() can print 1MB */
    EXPECT_EQ(sdslen(x), etalon_size);
    EXPECT_EQ(memcmp(x, etalon, etalon_size), 0);
    free(etalon);

    sdsfree(x);
    x = sdsnew("--");
    x = sdscatfmt(x, "Hello %s World %I,%I--", "Hi!", LLONG_MIN, LLONG_MAX);
    /* sdscatfmt() seems working in the base case */
    EXPECT_STREQ(x, "--Hello Hi! World -9223372036854775808,9223372036854775807--");

    sdsfree(x);
    x = sdsnew("--");
    x = sdscatfmt(x, "%u,%U--", UINT_MAX, ULLONG_MAX);
    /* sdscatfmt() seems working with unsigned numbers */
    EXPECT_STREQ(x, "--4294967295,18446744073709551615--");

    sdsfree(x);
    x = sdsnew(" x ");
    sdstrim(x, " x");
    /* sdstrim() works when all chars match */
    EXPECT_EQ(sdslen(x), 0);

    sdsfree(x);
    x = sdsnew(" x ");
    sdstrim(x, " ");
    /* sdstrim() works when a single char remains */
    EXPECT_EQ(sdslen(x), 1);
    EXPECT_EQ(x[0], 'x');

    sdsfree(x);
    x = sdsnew("xxciaoyyy");
    sdstrim(x, "xy");
    /* sdstrim() correctly trims characters */
    EXPECT_STREQ(x, "ciao");

    y = sdsdup(x);
    sdsrange(y, 1, 1);
    /* sdsrange(...,1,1) */
    EXPECT_STREQ(y, "i");

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y, 1, -1);
    /* sdsrange(...,1,-1) */
    EXPECT_STREQ(y, "iao");

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y, -2, -1);
    /* sdsrange(...,-2,-1) */
    EXPECT_STREQ(y, "ao");

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y, 2, 1);
    /* sdsrange(...,2,1) */
    EXPECT_STREQ(y, "");

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y, 1, 100);
    /* sdsrange(...,1,100) */
    EXPECT_STREQ(y, "iao");

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y, 100, 100);
    /* sdsrange(...,100,100) */
    EXPECT_STREQ(y, "");

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y, 4, 6);
    /* sdsrange(...,4,6) */
    EXPECT_STREQ(y, "");

    sdsfree(y);
    y = sdsdup(x);
    sdsrange(y, 3, 6);
    /* sdsrange(...,3,6) */
    EXPECT_STREQ(y, "o");

    sdsfree(y);
    sdsfree(x);
    x = sdsnew("foo");
    y = sdsnew("foa");
    /* sdscmp(foo,foa) */
    EXPECT_GT(sdscmp(x, y), 0);

    sdsfree(y);
    sdsfree(x);
    x = sdsnew("bar");
    y = sdsnew("bar");
    /* sdscmp(bar,bar) */
    EXPECT_EQ(sdscmp(x, y), 0);

    sdsfree(y);
    sdsfree(x);
    x = sdsnew("aar");
    y = sdsnew("bar");
    /* sdscmp(bar,bar) */
    EXPECT_LT(sdscmp(x, y), 0);

    sdsfree(y);
    sdsfree(x);
    x = sdsnewlen("\a\n\0foo\r", 7);
    y = sdscatrepr(sdsempty(), x, sdslen(x));
    /* sdscatrepr(...data...) */
    EXPECT_EQ(memcmp(y, "\"\\a\\n\\x00foo\\r\"", 15), 0);

    unsigned int oldfree;
    char *p;
    int i;
    size_t step = 10, j;

    sdsfree(x);
    sdsfree(y);
    x = sdsnew("0");
    /* sdsnew() free/len buffers */
    EXPECT_EQ(sdslen(x), 1);
    EXPECT_EQ(sdsavail(x), 0);

    /* Run the test a few times in order to hit the first two SDS header types. */
    for (i = 0; i < 10; i++) {
        size_t oldlen = sdslen(x);
        x = sdsMakeRoomFor(x, step);
        int type = x[-1] & SDS_TYPE_MASK;

        /* sdsMakeRoomFor() len */
        EXPECT_EQ(sdslen(x), oldlen);
        if (type != SDS_TYPE_5) {
            /* sdsMakeRoomFor() free */
            EXPECT_GE(sdsavail(x), step);
            oldfree = sdsavail(x);
            (void)oldfree;
        }
        p = x + oldlen;
        for (j = 0; j < step; j++) {
            p[j] = 'A' + j;
        }
        sdsIncrLen(x, step);
    }
    /* sdsMakeRoomFor() content */
    EXPECT_STREQ(x, "0ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGH"
                     "IJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ");
    /* sdsMakeRoomFor() final length */
    EXPECT_EQ(sdslen(x), 101);

    sdsfree(x);

    /* Simple template */
    x = sdstemplate("v1={variable1} v2={variable2}", sdsTestTemplateCallback, NULL);
    /* sdstemplate() normal flow */
    EXPECT_STREQ(x, "v1=value1 v2=value2");
    sdsfree(x);

    /* Template with callback error */
    x = sdstemplate("v1={variable1} v3={doesnotexist}", sdsTestTemplateCallback, NULL);
    /* sdstemplate() with callback error */
    EXPECT_EQ(x, nullptr);

    /* Template with empty var name */
    x = sdstemplate("v1={", sdsTestTemplateCallback, NULL);
    /* sdstemplate() with empty var name */
    EXPECT_EQ(x, nullptr);

    /* Template with truncated var name */
    x = sdstemplate("v1={start", sdsTestTemplateCallback, NULL);
    /* sdstemplate() with truncated var name */
    EXPECT_EQ(x, nullptr);

    /* Template with quoting */
    x = sdstemplate("v1={{{variable1}} {{} v2={variable2}", sdsTestTemplateCallback, NULL);
    /* sdstemplate() with quoting */
    EXPECT_STREQ(x, "v1={value1} {} v2=value2");
    sdsfree(x);

    /* Test sdsResize - extend */
    x = sdsnew("1234567890123456789012345678901234567890");
    x = sdsResize(x, 200, 1);
    /* sdsReszie() expand type */
    EXPECT_EQ(x[-1], SDS_TYPE_8);
    /* sdsReszie() expand len */
    EXPECT_EQ(sdslen(x), 40);
    /* sdsReszie() expand strlen */
    EXPECT_EQ(strlen(x), 40);
    /* Different allocator allocates at least as large as requested size,
     * to confirm the allocator won't waste too much,
     * we add a largest size checker here. */
    /* sdsReszie() expand alloc */
    EXPECT_GE(sdsalloc(x), 200);
    EXPECT_LT(sdsalloc(x), 400);
    /* Test sdsResize - trim free space */
    x = sdsResize(x, 80, 1);
    /* sdsReszie() shrink type */
    EXPECT_EQ(x[-1], SDS_TYPE_8);
    /* sdsReszie() shrink len */
    EXPECT_EQ(sdslen(x), 40);
    /* sdsReszie() shrink strlen */
    EXPECT_EQ(strlen(x), 40);
    /* sdsReszie() shrink alloc */
    EXPECT_GE(sdsalloc(x), 80);
    /* Test sdsResize - crop used space */
    x = sdsResize(x, 30, 1);
    /* sdsReszie() crop type */
    EXPECT_EQ(x[-1], SDS_TYPE_8);
    /* sdsReszie() crop len */
    EXPECT_EQ(sdslen(x), 30);
    /* sdsReszie() crop strlen */
    EXPECT_EQ(strlen(x), 30);
    /* sdsReszie() crop alloc */
    EXPECT_GE(sdsalloc(x), 30);
    /* Test sdsResize - extend to different class */
    x = sdsResize(x, 400, 1);
    /* sdsReszie() expand type */
    EXPECT_EQ(x[-1], SDS_TYPE_16);
    /* sdsReszie() expand len */
    EXPECT_EQ(sdslen(x), 30);
    /* sdsReszie() expand strlen */
    EXPECT_EQ(strlen(x), 30);
    /* sdsReszie() expand alloc */
    EXPECT_GE(sdsalloc(x), 400);
    /* Test sdsResize - shrink to different class */
    x = sdsResize(x, 4, 1);
    /* sdsReszie() crop type */
    EXPECT_EQ(x[-1], SDS_TYPE_8);
    /* sdsReszie() crop len */
    EXPECT_EQ(sdslen(x), 4);
    /* sdsReszie() crop strlen */
    EXPECT_EQ(strlen(x), 4);
    /* sdsReszie() crop alloc */
    EXPECT_GE(sdsalloc(x), 4);
    sdsfree(x);
}

TEST_F(SdsTest, TestTypesAndAllocSize) {
    sds x = sdsnewlen(NULL, 31);
    /* len 31 type */
    EXPECT_EQ((x[-1] & SDS_TYPE_MASK), SDS_TYPE_5);
    /* len 31 sdsAllocSize */
    EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
    sdsfree(x);

    x = sdsnewlen(NULL, 32);
    /* len 32 type */
    EXPECT_GE((x[-1] & SDS_TYPE_MASK), SDS_TYPE_8);
    /* len 32 sdsAllocSize */
    EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
    sdsfree(x);

    x = sdsnewlen(NULL, 252);
    /* len 252 type */
    EXPECT_GE((x[-1] & SDS_TYPE_MASK), SDS_TYPE_8);
    /* len 252 sdsAllocSize */
    EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
    sdsfree(x);

    x = sdsnewlen(NULL, 253);
    /* len 253 type */
    EXPECT_EQ((x[-1] & SDS_TYPE_MASK), SDS_TYPE_16);
    /* len 253 sdsAllocSize */
    EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
    sdsfree(x);

    x = sdsnewlen(NULL, 65530);
    /* len 65530 type */
    EXPECT_GE((x[-1] & SDS_TYPE_MASK), SDS_TYPE_16);
    /* len 65530 sdsAllocSize */
    EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
    sdsfree(x);

    x = sdsnewlen(NULL, 65531);
    /* len 65531 type */
    EXPECT_GE((x[-1] & SDS_TYPE_MASK), SDS_TYPE_32);
    /* len 65531 sdsAllocSize */
    EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
    sdsfree(x);

#if (LONG_MAX == LLONG_MAX)
    if (testing::GTEST_FLAG(filter) == "*TestTypesAndAllocSize*") {
        x = sdsnewlen(NULL, 4294967286);
        /* len 4294967286 type */
        EXPECT_GE((x[-1] & SDS_TYPE_MASK), SDS_TYPE_32);
        /* len 4294967286 sdsAllocSize */
        EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
        sdsfree(x);

        x = sdsnewlen(NULL, 4294967287);
        /* len 4294967287 type */
        EXPECT_EQ((x[-1] & SDS_TYPE_MASK), SDS_TYPE_64);
        /* len 4294967287 sdsAllocSize */
        EXPECT_EQ(sdsAllocSize(x), s_malloc_usable_size(sdsAllocPtr(x)));
        sdsfree(x);
    }
#endif
}

/* The test verifies that we can adjust SDS types if an allocator returned
 * larger buffer. The maximum length for type SDS_TYPE_X is
 * 2^X - header_size(SDS_TYPE_X) - 1. The maximum value to be stored in alloc
 * field is 2^X - 1. When allocated buffer is larger than
 * 2^X + header_size(SDS_TYPE_X), we "move" to a larger type SDS_TYPE_Y. To be
 * sure SDS_TYPE_Y header fits into 2^X + header_size(SDS_TYPE_X) + 1 bytes, the
 * difference between header sizes must be smaller than
 * header_size(SDS_TYPE_X) + 1.
 * We ignore SDS_TYPE_5 as it doesn't have alloc field. */
TEST_F(SdsTest, TestSdsHeaderSizes) {
    /* can't always adjust SDS_TYPE_8 with SDS_TYPE_16 */
    EXPECT_LE(sizeof(struct sdshdr16), 2 * sizeof(struct sdshdr8) + 1);
    /* can't always adjust SDS_TYPE_16 with SDS_TYPE_32 */
    EXPECT_LE(sizeof(struct sdshdr32), 2 * sizeof(struct sdshdr16) + 1);
#if (LONG_MAX == LLONG_MAX)
    /* can't always adjust SDS_TYPE_32 with SDS_TYPE_64 */
    EXPECT_LE(sizeof(struct sdshdr64), 2 * sizeof(struct sdshdr32) + 1);
#endif
}

TEST_F(SdsTest, TestSdssplitargs) {
    int len;
    sds *sargv;

    sargv = sdssplitargs("Testing one two three", &len);
    EXPECT_EQ(len, 4);
    EXPECT_STREQ(sargv[0], "Testing");
    EXPECT_STREQ(sargv[1], "one");
    EXPECT_STREQ(sargv[2], "two");
    EXPECT_STREQ(sargv[3], "three");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("", &len);
    EXPECT_EQ(len, 0);
    EXPECT_NE(sargv, nullptr);
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("\"Testing split strings\" 'Another split string'", &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "Testing split strings");
    EXPECT_STREQ(sargv[1], "Another split string");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("\"Hello\" ", &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "Hello");
    sdsfreesplitres(sargv, len);

    const char *binary_string = "\"\\x73\\x75\\x70\\x65\\x72\\x20\\x00\\x73\\x65\\x63\\x72\\x65\\x74\\x20\\x70\\x61\\x73\\x73\\x77\\x6f\\x72\\x64\"";
    sargv = sdssplitargs(binary_string, &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "super \x00secret password");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("unquoted", &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "unquoted");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("empty string \"\"", &len);
    EXPECT_EQ(len, 3);
    EXPECT_STREQ(sargv[0], "empty");
    EXPECT_STREQ(sargv[1], "string");
    EXPECT_STREQ(sargv[2], "");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("\"deeply\\\"quoted\" 's\\'t\\\"r'ing", &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "deeply\"quoted");
    EXPECT_STREQ(sargv[1], "s't\\\"ring");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("unquoted\" \"with' 'quotes string", &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "unquoted with quotes");
    EXPECT_STREQ(sargv[1], "string");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("\"quoted\"' another 'quoted string", &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "quoted another quoted");
    EXPECT_STREQ(sargv[1], "string");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("\"shell-like \"'\"'\"'\"' quote-escaping '", &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "shell-like \"' quote-escaping ");
    sdsfreesplitres(sargv, len);

    sargv = sdssplitargs("\"unterminated \"'single quotes", &len);
    EXPECT_EQ(len, 0);
    EXPECT_EQ(sargv, nullptr);

    sargv = sdssplitargs("'unterminated '\"double quotes", &len);
    EXPECT_EQ(len, 0);
    EXPECT_EQ(sargv, nullptr);
}

TEST_F(SdsTest, TestSdsnsplitargs) {
    int len;
    sds *sargv;
    const char *test_str;

    /* Test basic parameter splitting */
    test_str = "Testing one two three";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 4);
    EXPECT_STREQ(sargv[0], "Testing");
    EXPECT_STREQ(sargv[1], "one");
    EXPECT_STREQ(sargv[2], "two");
    EXPECT_STREQ(sargv[3], "three");
    sdsfreesplitres(sargv, len);

    /* Test empty string */
    sargv = sdsnsplitargs("", 0, &len);
    EXPECT_EQ(len, 0);
    EXPECT_NE(sargv, nullptr);
    sdsfreesplitres(sargv, len);

    /* Test quoted strings */
    test_str = "\"Testing split strings\" 'Another split string'";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "Testing split strings");
    EXPECT_STREQ(sargv[1], "Another split string");
    sdsfreesplitres(sargv, len);

    /* Test trailing space after quoted string */
    test_str = "\"Hello\" ";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "Hello");
    sdsfreesplitres(sargv, len);

    /* Test binary string with null character using \x escape */
    test_str = "\"\\x73\\x75\\x70\\x65\\x72\\x20\\x00\\x73\\x65\\x63\\x72\\x65\\x74\\x20\\x70\\x61\\x73\\x73\\x77\\x6f\\x72\\x64\"";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "super \x00secret password");
    sdsfreesplitres(sargv, len);

    char str_with_null[] = "test\0null";
    sargv = sdsnsplitargs(str_with_null, sizeof(str_with_null) - 1, &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "test");
    sdsfreesplitres(sargv, len);

    /* Test single unquoted string */
    test_str = "unquoted";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "unquoted");
    sdsfreesplitres(sargv, len);

    /* Test empty quoted string */
    test_str = "empty string \"\"";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 3);
    EXPECT_STREQ(sargv[0], "empty");
    EXPECT_STREQ(sargv[1], "string");
    EXPECT_STREQ(sargv[2], "");
    sdsfreesplitres(sargv, len);

    /* Test escaped quotes */
    test_str = "\"deeply\\\"quoted\" 's\\'t\\\"r'ing";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "deeply\"quoted");
    EXPECT_STREQ(sargv[1], "s't\\\"ring");
    sdsfreesplitres(sargv, len);

    /* Test mixed quoted and unquoted parts */
    test_str = "unquoted\" \"with' 'quotes string";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "unquoted with quotes");
    EXPECT_STREQ(sargv[1], "string");
    sdsfreesplitres(sargv, len);

    /* Test concatenated quoted strings */
    test_str = "\"quoted\"' another 'quoted string";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "quoted another quoted");
    EXPECT_STREQ(sargv[1], "string");
    sdsfreesplitres(sargv, len);

    /* Test complex quote escaping */
    test_str = "\"shell-like \"'\"'\"'\"' quote-escaping '";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "shell-like \"' quote-escaping ");
    sdsfreesplitres(sargv, len);

    /* Test unterminated double quote */
    test_str = "\"unterminated \"'single quotes";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 0);
    EXPECT_EQ(sargv, nullptr);

    /* Test unterminated single quote */
    test_str = "'unterminated '\"double quotes";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 0);
    EXPECT_EQ(sargv, nullptr);

    /* Test partial string length (truncated input) */
    test_str = "Testing one two three";
    sargv = sdsnsplitargs(test_str, 8, &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "Testing");
    sdsfreesplitres(sargv, len);

    /* Test string with exact length (no truncation) */
    test_str = "Exact length";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "Exact");
    EXPECT_STREQ(sargv[1], "length");
    sdsfreesplitres(sargv, len);

    /* Test string with leading spaces */
    test_str = "   leading spaces";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "leading");
    EXPECT_STREQ(sargv[1], "spaces");
    sdsfreesplitres(sargv, len);

    /* Test string with trailing spaces */
    test_str = "trailing spaces   ";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "trailing");
    EXPECT_STREQ(sargv[1], "spaces");
    sdsfreesplitres(sargv, len);

    /* Test string with consecutive space */
    test_str = "multiple   spaces";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "multiple");
    EXPECT_STREQ(sargv[1], "spaces");
    sdsfreesplitres(sargv, len);

    /* Test string with only spaces */
    test_str = "   ";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 0);
    EXPECT_NE(sargv, nullptr);
    sdsfreesplitres(sargv, len);

    /* Test string containing null character in the middle of parsing */
    char str_with_null_in_middle[] = "arg1\0arg2 arg3";
    sargv = sdsnsplitargs(str_with_null_in_middle, sizeof(str_with_null_in_middle) - 1, &len);
    EXPECT_EQ(len, 1);
    EXPECT_STREQ(sargv[0], "arg1");
    sdsfreesplitres(sargv, len);

    /* Test very long single argument */
    char long_arg[1024];
    memset(long_arg, 'a', sizeof(long_arg) - 1);
    long_arg[sizeof(long_arg) - 1] = '\0';
    sargv = sdsnsplitargs(long_arg, sizeof(long_arg) - 1, &len);
    EXPECT_EQ(len, 1);
    EXPECT_EQ(strlen(sargv[0]), sizeof(long_arg) - 1);
    EXPECT_STREQ(sargv[0], long_arg);
    sdsfreesplitres(sargv, len);

    /* Test mixed quote types in one argument */
    test_str = "\"double'quotes\" 'single\"quotes'";
    sargv = sdsnsplitargs(test_str, strlen(test_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "double'quotes");
    EXPECT_STREQ(sargv[1], "single\"quotes");
    sdsfreesplitres(sargv, len);

    /* Test mixed quote types with different lengths */
    sds complex_str = sdsnew("\"double'quotes\" 'single\"quotes'");
    sargv = sdsnsplitargs(complex_str, sdslen(complex_str) - 1, &len);
    EXPECT_EQ(len, 0);

    complex_str = sdscatlen(complex_str, "\0", 1);
    sargv = sdsnsplitargs(complex_str, sdslen(complex_str) - 1, &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "double'quotes");
    EXPECT_STREQ(sargv[1], "single\"quotes");
    sdsfreesplitres(sargv, len);

    sargv = sdsnsplitargs(complex_str, sdslen(complex_str), &len);
    EXPECT_EQ(len, 2);
    EXPECT_STREQ(sargv[0], "double'quotes");
    EXPECT_STREQ(sargv[1], "single\"quotes");
    sdsfreesplitres(sargv, len);

    sargv = sdsnsplitargs(complex_str, sdslen(complex_str) - 2, &len);
    EXPECT_EQ(len, 0);
    sdsfree(complex_str);
}

/* This test is disabled by default because it takes a long time to run (1M iterations).
 * It's used for performance comparison between sdsnsplitargs and sdssplitargs.
 * To run this test explicitly, use:
 *   ./src/gtest/valkey-unit-gtests --gtest_filter=SdsTest.DISABLED_TestSdsnsplitargsBenchmark --gtest_also_run_disabled_tests */
TEST_F(SdsTest, DISABLED_TestSdsnsplitargsBenchmark) {
    char str_with_null_in_middle[] = "arg1\0arg2 arg3";
    size_t str_len = sizeof(str_with_null_in_middle) - 1;
    int len = 0;

    long long start = ustime();
    for (int i = 0; i < 1000000; i++) {
        sds *sargv = sdsnsplitargs(str_with_null_in_middle, str_len, &len);
        sdsfreesplitres(sargv, len);
    }
    printf("sdsnsplitargs 1000000 times: %f\n", static_cast<double>(ustime() - start) / 1000000);

    start = ustime();
    for (int i = 0; i < 1000000; i++) {
        sds str = sdsnewlen(str_with_null_in_middle, str_len);
        sds *sargv = sdssplitargs(str, &len);
        sdsfreesplitres(sargv, len);
        sdsfree(str);
    }
    printf("sdssplitargs 1000000 times: %f\n", static_cast<double>(ustime() - start) / 1000000);
}

/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

extern "C" {
#include "zipmap.h"
}

class ZipmapTest : public ::testing::Test {
};

TEST_F(ZipmapTest, zipmapIterateWithLargeKey) {
    char zm[] = "\x04"
                "\x04"
                "name"
                "\x03\x00"
                "foo"
                "\x07"
                "surname"
                "\x03\x00"
                "foo"
                "\x05"
                "noval"
                "\x00\x00"
                "\xfe\x00\x02\x00\x00"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "\x04\x00"
                "long"
                "\xff";
    EXPECT_TRUE(zipmapValidateIntegrity(reinterpret_cast<unsigned char *>(zm), sizeof zm - 1, 1));

    unsigned char *p = zipmapRewind(reinterpret_cast<unsigned char *>(zm));
    unsigned char *key, *value;
    unsigned int klen, vlen;
    char buf[512];
    memset(buf, 'a', 512);
    char *expected_key[] = {const_cast<char *>("name"), const_cast<char *>("surname"), const_cast<char *>("noval"), buf};
    char *expected_value[] = {const_cast<char *>("foo"), const_cast<char *>("foo"), nullptr, const_cast<char *>("long")};
    unsigned int expected_klen[] = {4, 7, 5, 512};
    unsigned int expected_vlen[] = {3, 3, 0, 4};
    int iter = 0;

    while ((p = zipmapNext(p, &key, &klen, &value, &vlen)) != nullptr) {
        char *tmp = expected_key[iter];
        EXPECT_EQ(klen, expected_klen[iter]);
        EXPECT_EQ(strncmp(tmp, reinterpret_cast<const char *>(key), klen), 0);
        tmp = expected_value[iter];
        EXPECT_EQ(vlen, expected_vlen[iter]);
        EXPECT_EQ(strncmp(tmp, reinterpret_cast<const char *>(value), vlen), 0);
        iter++;
    }
}

TEST_F(ZipmapTest, zipmapIterateThroughElements) {
    char zm[] = "\x06"
                "\x04"
                "name"
                "\x03\x00"
                "foo"
                "\x07"
                "surname"
                "\x03\x00"
                "foo"
                "\x03"
                "age"
                "\x03\x00"
                "foo"
                "\x05"
                "hello"
                "\x06\x00"
                "world!"
                "\x03"
                "foo"
                "\x05\x00"
                "12345"
                "\x05"
                "noval"
                "\x00\x00"
                "\xff";
    EXPECT_TRUE(zipmapValidateIntegrity(reinterpret_cast<unsigned char *>(zm), sizeof zm - 1, 1));

    unsigned char *i = zipmapRewind(reinterpret_cast<unsigned char *>(zm));
    unsigned char *key, *value;
    unsigned int klen, vlen;
    char *expected_key[] = {const_cast<char *>("name"), const_cast<char *>("surname"), const_cast<char *>("age"), const_cast<char *>("hello"), const_cast<char *>("foo"), const_cast<char *>("noval")};
    char *expected_value[] = {const_cast<char *>("foo"), const_cast<char *>("foo"), const_cast<char *>("foo"), const_cast<char *>("world!"), const_cast<char *>("12345"), const_cast<char *>("")};
    unsigned int expected_klen[] = {4, 7, 3, 5, 3, 5};
    unsigned int expected_vlen[] = {3, 3, 3, 6, 5, 0};
    int iter = 0;

    while ((i = zipmapNext(i, &key, &klen, &value, &vlen)) != nullptr) {
        char *tmp = expected_key[iter];
        EXPECT_EQ(klen, expected_klen[iter]);
        EXPECT_EQ(strncmp(tmp, reinterpret_cast<const char *>(key), klen), 0);
        tmp = expected_value[iter];
        EXPECT_EQ(vlen, expected_vlen[iter]);
        EXPECT_EQ(strncmp(tmp, reinterpret_cast<const char *>(value), vlen), 0);
        iter++;
    }
}

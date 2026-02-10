/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>

extern "C" {
#include "server.h"
}

class ObjectTest : public ::testing::Test {
};

TEST_F(ObjectTest, object_with_key) {
    sds key = sdsnew("foo");
    robj *val = createStringObject("bar", strlen("bar"));
    EXPECT_EQ(val->encoding, static_cast<unsigned>(OBJ_ENCODING_EMBSTR));
    EXPECT_EQ(sdslen(static_cast<sds>(objectGetVal(val))), 3u);

    /* Prevent objectSetKeyAndExpire from freeing the old val when reallocating it. */
    incrRefCount(val);

    robj *o = objectSetKeyAndExpire(val, key, -1);
    EXPECT_EQ(o->encoding, static_cast<unsigned>(OBJ_ENCODING_EMBSTR));
    EXPECT_NE(objectGetKey(o), nullptr);

    /* Check embedded key "foo" */
    EXPECT_EQ(sdslen(objectGetKey(o)), 3u);
    EXPECT_EQ(sdslen(key), 3u);
    EXPECT_EQ(sdscmp(objectGetKey(o), key), 0);
    EXPECT_EQ(strcmp(objectGetKey(o), "foo"), 0);

    /* Check embedded value "bar" (EMBSTR content) */
    EXPECT_EQ(sdscmp(static_cast<sds>(objectGetVal(o)), static_cast<sds>(objectGetVal(val))), 0);
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(o)), "bar"), 0);
    EXPECT_EQ(sdslen(static_cast<sds>(objectGetVal(o))), 3u);

    /* Either they're two separate objects, or one object with refcount == 2. */
    if (o == val) {
        EXPECT_EQ(static_cast<unsigned>(o->refcount), 2u);
    } else {
        EXPECT_EQ(static_cast<unsigned>(o->refcount), 1u);
        EXPECT_EQ(static_cast<unsigned>(val->refcount), 1u);
    }

    /* Free them. */
    sdsfree(key);
    decrRefCount(val);
    decrRefCount(o);
}

TEST_F(ObjectTest, embedded_string_with_key) {
    /* key of length 32 - type 8 */
    sds key = sdsnew("k:123456789012345678901234567890");
    EXPECT_EQ(sdslen(key), 32u);

    /* 32B key and 15B value should be embedded within 64B. Contents:
     * - 8B robj (no ptr) + 1B key header size
     * - 3B key header + 32B key + 1B null terminator
     * - 3B val header + 15B val + 1B null terminator
     * because no pointers are stored, there is no difference for 32 bit builds*/
    const char *short_value = "123456789012345";
    EXPECT_EQ(strlen(short_value), 15u);
    robj *short_val_obj = createStringObject(short_value, strlen(short_value));
    robj *embstr_obj = objectSetKeyAndExpire(short_val_obj, key, -1);
    EXPECT_EQ(embstr_obj->encoding, static_cast<unsigned>(OBJ_ENCODING_EMBSTR));
    EXPECT_EQ(sdslen(objectGetKey(embstr_obj)), 32u);
    EXPECT_EQ(sdscmp(objectGetKey(embstr_obj), key), 0);
    EXPECT_EQ(sdslen(static_cast<sds>(objectGetVal(embstr_obj))), 15u);
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(embstr_obj)), short_value), 0);

    /* value of length 16 cannot be embedded with other contents within 64B */
    const char *longer_value = "1234567890123456";
    EXPECT_EQ(strlen(longer_value), 16u);
    robj *longer_val_obj = createStringObject(longer_value, strlen(longer_value));
    robj *raw_obj = objectSetKeyAndExpire(longer_val_obj, key, -1);
    EXPECT_EQ(raw_obj->encoding, static_cast<unsigned>(OBJ_ENCODING_RAW));
    EXPECT_EQ(sdslen(objectGetKey(raw_obj)), 32u);
    EXPECT_EQ(sdscmp(objectGetKey(raw_obj), key), 0);
    EXPECT_EQ(sdslen(static_cast<sds>(objectGetVal(raw_obj))), 16u);
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(raw_obj)), longer_value), 0);

    sdsfree(key);
    decrRefCount(embstr_obj);
    decrRefCount(raw_obj);
}

TEST_F(ObjectTest, embedded_string_with_key_and_expire) {
    /* key of length 32 - type 8 */
    sds key = sdsnew("k:123456789012345678901234567890");
    EXPECT_EQ(sdslen(key), 32u);

    /* 32B key and 7B value should be embedded within 64B. Contents:
     * - 8B robj (no ptr) + 8B expire + 1B key header size
     * - 3B key header + 32B key + 1B null terminator
     * - 3B val header + 7B val + 1B null terminator
     * because no pointers are stored, there is no difference for 32 bit builds*/
    const char *short_value = "1234567";
    EXPECT_EQ(strlen(short_value), 7u);
    robj *short_val_obj = createStringObject(short_value, strlen(short_value));
    robj *embstr_obj = objectSetKeyAndExpire(short_val_obj, key, 128);
    EXPECT_EQ(embstr_obj->encoding, static_cast<unsigned>(OBJ_ENCODING_EMBSTR));
    EXPECT_EQ(sdslen(objectGetKey(embstr_obj)), 32u);
    EXPECT_EQ(sdscmp(objectGetKey(embstr_obj), key), 0);
    EXPECT_EQ(sdslen(static_cast<sds>(objectGetVal(embstr_obj))), 7u);
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(embstr_obj)), short_value), 0);

    /* value of length 8 cannot be embedded with other contents within 64B */
    const char *longer_value = "12345678";
    EXPECT_EQ(strlen(longer_value), 8u);
    robj *longer_val_obj = createStringObject(longer_value, strlen(longer_value));
    robj *raw_obj = objectSetKeyAndExpire(longer_val_obj, key, 128);
    EXPECT_EQ(raw_obj->encoding, static_cast<unsigned>(OBJ_ENCODING_RAW));
    EXPECT_EQ(sdslen(objectGetKey(raw_obj)), 32u);
    EXPECT_EQ(sdscmp(objectGetKey(raw_obj), key), 0);
    EXPECT_EQ(sdslen(static_cast<sds>(objectGetVal(raw_obj))), 8u);
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(raw_obj)), longer_value), 0);

    sdsfree(key);
    decrRefCount(embstr_obj);
    decrRefCount(raw_obj);
}

TEST_F(ObjectTest, embedded_value) {
    /* with only value there is only 12B overhead, so we can embed up to 52B.
     * 8B robj (no ptr) + 3B val header + 52B val + 1B null terminator */
    const char *val = "v:12345678901234567890123456789012345678901234567890";
    EXPECT_EQ(strlen(val), 52u);
    robj *embstr_obj = createStringObject(val, strlen(val));
    EXPECT_EQ(embstr_obj->encoding, static_cast<unsigned>(OBJ_ENCODING_EMBSTR));
    EXPECT_EQ(sdslen(static_cast<sds>(objectGetVal(embstr_obj))), 52u);
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(embstr_obj)), val), 0);

    decrRefCount(embstr_obj);
}

TEST_F(ObjectTest, unembed_value) {
    const char *short_value = "embedded value";
    robj *short_val_obj = createStringObject(short_value, strlen(short_value));
    sds key = sdsnew("embedded key");
    long long expire = 155;

    robj *obj = objectSetKeyAndExpire(short_val_obj, key, expire);
    EXPECT_EQ(obj->encoding, static_cast<unsigned>(OBJ_ENCODING_EMBSTR));
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(obj)), short_value), 0);
    EXPECT_EQ(sdscmp(objectGetKey(obj), key), 0);
    EXPECT_EQ(objectGetExpire(obj), expire);
    EXPECT_NE(objectGetVal(obj), short_value);

    /* Unembed the value - it uses a separate allocation now.
     * the other embedded data gets shifted, so check them too */
    objectUnembedVal(obj);
    EXPECT_EQ(obj->encoding, static_cast<unsigned>(OBJ_ENCODING_RAW));
    EXPECT_EQ(strcmp(static_cast<const char *>(objectGetVal(obj)), short_value), 0);
    EXPECT_EQ(sdscmp(objectGetKey(obj), key), 0);
    EXPECT_EQ(objectGetExpire(obj), expire);
    EXPECT_NE(objectGetVal(obj), short_value); /* different allocation, different copy */

    sdsfree(key);
    decrRefCount(obj);
}

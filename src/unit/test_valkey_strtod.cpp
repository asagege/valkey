/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <cerrno>
#include <cmath>

extern "C" {
#include "valkey_strtod.h"
}

class ValkeyStrtodTest : public ::testing::Test {};

TEST_F(ValkeyStrtodTest, TestValkeyStrtod) {
    errno = 0;
    double value = valkey_strtod("231.2341234", NULL);
    ASSERT_EQ(value, 231.2341234);
    ASSERT_EQ(errno, 0);

    value = valkey_strtod("+inf", NULL);
    ASSERT_TRUE(std::isinf(value));
    ASSERT_EQ(errno, 0);

    value = valkey_strtod("-inf", NULL);
    ASSERT_TRUE(std::isinf(value));
    ASSERT_EQ(errno, 0);

    value = valkey_strtod("inf", NULL);
    ASSERT_TRUE(std::isinf(value));
    ASSERT_EQ(errno, 0);
}

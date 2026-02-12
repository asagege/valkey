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
    double value = valkey_strtod("231.2341234", nullptr);
    EXPECT_DOUBLE_EQ(value, 231.2341234);
    EXPECT_EQ(errno, 0);

    value = valkey_strtod("+inf", nullptr);
    EXPECT_TRUE(std::isinf(value));
    EXPECT_EQ(errno, 0);

    value = valkey_strtod("-inf", nullptr);
    EXPECT_TRUE(std::isinf(value));
    EXPECT_EQ(errno, 0);

    value = valkey_strtod("inf", nullptr);
    EXPECT_TRUE(std::isinf(value));
    EXPECT_EQ(errno, 0);
}

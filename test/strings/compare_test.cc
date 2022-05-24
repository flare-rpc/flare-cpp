// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "flare/strings/compare.h"
#include "testing/gtest_wrap.h"

namespace {

    TEST(MatchTest, EqualsIgnoreCase) {
        std::string text = "the";
        std::string_view data(text);

        EXPECT_TRUE(flare::equal_case(data, "The"));
        EXPECT_TRUE(flare::equal_case(data, "THE"));
        EXPECT_TRUE(flare::equal_case(data, "the"));
        EXPECT_FALSE(flare::equal_case(data, "Quick"));
        EXPECT_FALSE(flare::equal_case(data, "then"));
    }

}

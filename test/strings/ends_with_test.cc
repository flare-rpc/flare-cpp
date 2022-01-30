// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "flare/strings/ends_with.h"
#include "gtest/gtest.h"

namespace {

    TEST(MatchTest, ends_with_case) {
        EXPECT_TRUE(flare::strings::ends_with_case("foo", "foo"));
        EXPECT_TRUE(flare::strings::ends_with_case("foo", "Oo"));
        EXPECT_TRUE(flare::strings::ends_with_case("foo", ""));
        EXPECT_FALSE(flare::strings::ends_with_case("foo", "fooo"));
        EXPECT_FALSE(flare::strings::ends_with_case("", "fo"));
    }

    TEST(MatchTest, ends_with) {
        const std::string s1("123\0abc", 7);
        const std::string_view a("foobar");
        const std::string_view b(s1);
        const std::string_view e;
        EXPECT_TRUE(flare::strings::ends_with(a, a));
        EXPECT_TRUE(flare::strings::ends_with(a, "bar"));
        EXPECT_TRUE(flare::strings::ends_with(a, e));
        EXPECT_TRUE(flare::strings::ends_with(b, s1));
        EXPECT_TRUE(flare::strings::ends_with(b, b));
        EXPECT_TRUE(flare::strings::ends_with(b, e));
        EXPECT_TRUE(flare::strings::ends_with(e, ""));
        EXPECT_FALSE(flare::strings::ends_with(a, b));
        EXPECT_FALSE(flare::strings::ends_with(b, a));
        EXPECT_FALSE(flare::strings::ends_with(e, a));
    }

}

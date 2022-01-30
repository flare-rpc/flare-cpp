// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "flare/strings/trim.h"
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>
#include "gtest/gtest.h"
#include "flare/base/profile.h"

TEST(trim_left, FromStringView) {
    EXPECT_EQ(std::string_view{},
              flare::strings::trim_left(std::string_view{}));
    EXPECT_EQ("foo", flare::strings::trim_left({"foo"}));
    EXPECT_EQ("foo", flare::strings::trim_left({"\t  \n\f\r\n\vfoo"}));
    EXPECT_EQ("foo foo\n ",
              flare::strings::trim_left({"\t  \n\f\r\n\vfoo foo\n "}));
    EXPECT_EQ(std::string_view{}, flare::strings::trim_left(
            {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_left, InPlace) {
    std::string str;

    flare::strings::trim_left(&str);
    EXPECT_EQ("", str);

    str = "foo";
    flare::strings::trim_left(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo";
    flare::strings::trim_left(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo foo\n ";
    flare::strings::trim_left(&str);
    EXPECT_EQ("foo foo\n ", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    flare::strings::trim_left(&str);
    EXPECT_EQ(std::string_view{}, str);
}

TEST(trim_right, FromStringView) {
    EXPECT_EQ(std::string_view{},
              flare::strings::trim_right(std::string_view{}));
    EXPECT_EQ("foo", flare::strings::trim_right({"foo"}));
    EXPECT_EQ("foo", flare::strings::trim_right({"foo\t  \n\f\r\n\v"}));
    EXPECT_EQ(" \nfoo foo",
              flare::strings::trim_right({" \nfoo foo\t  \n\f\r\n\v"}));
    EXPECT_EQ(std::string_view{}, flare::strings::trim_right(
            {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_right, InPlace) {
    std::string str;

    flare::strings::trim_right(&str);
    EXPECT_EQ("", str);

    str = "foo";
    flare::strings::trim_right(&str);
    EXPECT_EQ("foo", str);

    str = "foo\t  \n\f\r\n\v";
    flare::strings::trim_right(&str);
    EXPECT_EQ("foo", str);

    str = " \nfoo foo\t  \n\f\r\n\v";
    flare::strings::trim_right(&str);
    EXPECT_EQ(" \nfoo foo", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    flare::strings::trim_right(&str);
    EXPECT_EQ(std::string_view{}, str);
}

TEST(trim_all, FromStringView) {
    EXPECT_EQ(std::string_view{},
              flare::strings::trim_all(std::string_view{}));
    EXPECT_EQ("foo", flare::strings::trim_all({"foo"}));
    EXPECT_EQ("foo",
              flare::strings::trim_all({"\t  \n\f\r\n\vfoo\t  \n\f\r\n\v"}));
    EXPECT_EQ("foo foo", flare::strings::trim_all(
            {"\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v"}));
    EXPECT_EQ(std::string_view{},
              flare::strings::trim_all({"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(trim_all, InPlace) {
    std::string str;

    flare::strings::trim_all(&str);
    EXPECT_EQ("", str);

    str = "foo";
    flare::strings::trim_all(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo\t  \n\f\r\n\v";
    flare::strings::trim_all(&str);
    EXPECT_EQ("foo", str);

    str = "\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v";
    flare::strings::trim_all(&str);
    EXPECT_EQ("foo foo", str);

    str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
    flare::strings::trim_all(&str);
    EXPECT_EQ(std::string_view{}, str);
}

TEST(trim_complete, InPlace) {
    const char *inputs[] = {"No extra space",
                            "  Leading whitespace",
                            "Trailing whitespace  ",
                            "  Leading and trailing  ",
                            " Whitespace \t  in\v   middle  ",
                            "'Eeeeep!  \n Newlines!\n",
                            "nospaces",
                            "",
                            "\n\t a\t\n\nb \t\n"};

    const char *outputs[] = {
            "No extra space",
            "Leading whitespace",
            "Trailing whitespace",
            "Leading and trailing",
            "Whitespace in middle",
            "'Eeeeep! Newlines!",
            "nospaces",
            "",
            "a\nb",
    };
    const int NUM_TESTS = FLARE_ARRAY_SIZE(inputs);

    for (int i = 0; i < NUM_TESTS; i++) {
        std::string s(inputs[i]);
        flare::strings::trim_complete(&s);
        EXPECT_EQ(outputs[i], s);
    }
}

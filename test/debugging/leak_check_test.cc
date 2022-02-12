// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <string>

#include "gtest/gtest.h"
#include "flare/base/logging.h"
#include "flare/debugging/leak_check.h"

namespace {

    TEST(LeakCheckTest, DetectLeakSanitizer) {
#ifdef FLARE_EXPECT_LEAK_SANITIZER
        EXPECT_TRUE(flare::debugging::have_leak_sanitizer());
#else
        EXPECT_FALSE(flare::debugging::have_leak_sanitizer());
#endif
    }

    TEST(LeakCheckTest, IgnoreLeakSuppressesLeakedMemoryErrors) {
        auto foo = flare::debugging::ignore_leak(new std::string("some ignored leaked string"));
        LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

    TEST(LeakCheckTest, LeakCheckDisablerIgnoresLeak) {
        flare::debugging::leak_check_disabler disabler;
        auto foo = new std::string("some std::string leaked while checks are disabled");
        LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

}  // namespace

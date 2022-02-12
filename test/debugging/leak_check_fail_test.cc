// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include <memory>
#include "gtest/gtest.h"
#include "flare/base/logging.h"
#include "flare/debugging/leak_check.h"

namespace {

    TEST(LeakCheckTest, LeakMemory) {
        // This test is expected to cause lsan failures on program exit. Therefore the
        // test will be run only by leak_check_test.sh, which will verify a
        // failed exit code.

        char *foo = strdup("lsan should complain about this leaked string");
        LOG(INFO) << "Should detect leaked std::string " << foo;
    }

    TEST(LeakCheckTest, LeakMemoryAfterDisablerScope) {
        // This test is expected to cause lsan failures on program exit. Therefore the
        // test will be run only by external_leak_check_test.sh, which will verify a
        // failed exit code.
        { flare::debugging::leak_check_disabler disabler; }
        char *foo = strdup("lsan should also complain about this leaked string");
        LOG(INFO) << "Re-enabled leak detection.Should detect leaked std::string " << foo;
    }

}  // namespace

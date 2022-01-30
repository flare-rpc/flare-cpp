//
// Created by liyinbin on 2022/1/29.
//

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flare/base/at_exit.h"

#include <gtest/gtest.h>

namespace {

    int g_test_counter_1 = 0;
    int g_test_counter_2 = 0;

    void IncrementTestCounter1(void*) {
        ++g_test_counter_1;
    }

    void IncrementTestCounter2(void*) {
        ++g_test_counter_2;
    }

    void ZeroTestCounters() {
        g_test_counter_1 = 0;
        g_test_counter_2 = 0;
    }

    void ExpectCounter1IsZero(void* unused) {
        EXPECT_EQ(0, g_test_counter_1);
    }

    void ExpectParamIsNull(void* param) {
        EXPECT_EQ(static_cast<void*>(NULL), param);
    }

    void ExpectParamIsCounter(void* param) {
        EXPECT_EQ(&g_test_counter_1, param);
    }

}  // namespace

class AtExitTest : public testing::Test {
private:
    // Don't test the global at_exit_manager, because asking it to process its
    // AtExit callbacks can ruin the global state that other tests may depend on.
    flare::base::ShadowingAtExitManager exit_manager_;
};

TEST_F(AtExitTest, Basic) {
    ZeroTestCounters();
    flare::base::at_exit_manager::register_callback(&IncrementTestCounter1, NULL);
    flare::base::at_exit_manager::register_callback(&IncrementTestCounter2, NULL);
    flare::base::at_exit_manager::register_callback(&IncrementTestCounter1, NULL);

    EXPECT_EQ(0, g_test_counter_1);
    EXPECT_EQ(0, g_test_counter_2);
    flare::base::at_exit_manager::process_callbacks_now();
    EXPECT_EQ(2, g_test_counter_1);
    EXPECT_EQ(1, g_test_counter_2);
}

TEST_F(AtExitTest, LIFOOrder) {
    ZeroTestCounters();
    flare::base::at_exit_manager::register_callback(&IncrementTestCounter1, NULL);
    flare::base::at_exit_manager::register_callback(&ExpectCounter1IsZero, NULL);
    flare::base::at_exit_manager::register_callback(&IncrementTestCounter2, NULL);

    EXPECT_EQ(0, g_test_counter_1);
    EXPECT_EQ(0, g_test_counter_2);
    flare::base::at_exit_manager::process_callbacks_now();
    EXPECT_EQ(1, g_test_counter_1);
    EXPECT_EQ(1, g_test_counter_2);
}

TEST_F(AtExitTest, Param) {
    flare::base::at_exit_manager::register_callback(&ExpectParamIsNull, NULL);
    flare::base::at_exit_manager::register_callback(&ExpectParamIsCounter,
                                           &g_test_counter_1);
    flare::base::at_exit_manager::process_callbacks_now();
}

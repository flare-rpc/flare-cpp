//
// Created by liyinbin on 2022/7/5.
//


#include <iostream>
#include <queue>
#include "testing/gtest_wrap.h"
#include "flare/future/future.h"
#include <array>
#include <queue>


TEST(future, future_of_reference) {
    flare::promise<std::reference_wrapper<int>> p;
    flare::future <std::reference_wrapper<int>> f = p.get_future();

    int var = 0;

    p.set_value(var);

    f.finally([](flare::expected <std::reference_wrapper<int>> dst) { (*dst).get() = 4; });

    ASSERT_EQ(var, 4);
}
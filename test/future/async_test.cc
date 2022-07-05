//
// Created by liyinbin on 2022/7/5.
//

#include <queue>
#include "testing/gtest_wrap.h"
#include "flare/future/future.h"

TEST(future, async) {
    std::queue<std::function<void()>> queue;

    auto fut = flare::async(queue, []() { return 12; });

    EXPECT_EQ(1, queue.size());
    int dst = 0;
    fut.finally([&](flare::expected<int, std::exception_ptr> x) { dst = x.value(); });
    EXPECT_EQ(0, dst);

    queue.front()();
    queue.pop();

    EXPECT_EQ(12, dst);
}
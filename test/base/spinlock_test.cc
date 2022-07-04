//
// Created by liyinbin on 2022/2/15.
//

#include "flare/thread/spinlock.h"
#include <thread>
#include "flare/thread/latch.h"
#include "testing/gtest_wrap.h"

namespace flare {


    std::uint64_t counter{};

    TEST(Spinlock, All) {
        constexpr auto T = 100;
        constexpr auto N = 100000;
        std::thread ts[100];
        latch latch(1);
        spinlock splk;

        for (auto &&t : ts) {
            t = std::thread([&] {
                latch.wait();
                for (int i = 0; i != N; ++i) {
                    std::scoped_lock lk(splk);
                    ++counter;
                }
            });
        }
        latch.count_down();
        for (auto &&t : ts) {
            t.join();
        }
        ASSERT_EQ(T * N, counter);
    }
}  // namespace flare
//
// Created by liyinbin on 2022/2/22.
//

#include "flare/fiber/future.h"
#include "flare/fiber/fiber.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "flare/fiber/this_fiber.h"
#include "flare/fiber/async.h"
#include "flare/base/fast_rand.h"

using namespace std::literals;

namespace flare::fiber {

    TEST(Future, BlockingGet) {
        for (int i = 0; i != 200; ++i) {
            fiber fbs[100];
            for (auto &&f : fbs) {
                auto op = [](void *) -> void * {
                    auto v = fiber_future_get(fiber_async(launch_policy::eLazy, [] {
                        std::vector<int> v{1, 2, 3, 4, 5};
                        int round = flare::base::fast_rand_less_than(10);
                        for (int j = 0; j != round; ++j) {
                            flare::this_fiber::fiber_yield();
                        }
                        return v;
                    }));
                    EXPECT_EQ(v, ::testing::ElementsAre(1, 2, 3, 4, 5));
                    return nullptr;
                };
                f = fiber(op, nullptr);
            }
            for (auto &&f : fbs) {
                f.join();
            }
        }
    }

    TEST(Future, BlockingTryGetOk) {
        std::atomic<bool> f{};
        auto future = fiber_async([&] {
            flare::this_fiber::fiber_sleep_for(1000000);
            f = true;
        });
        auto ts = flare::base::microseconds_to_timespec(flare::base::cpuwide_time_us() + 1000000);
        ASSERT_FALSE(fiber_future_try_get_until(std::move(future), &ts));
        ASSERT_FALSE(f);
        flare::this_fiber::fiber_sleep_for(2000000);
        ASSERT_TRUE(f);
    }

}  // namespace abel

//
// Created by liyinbin on 2022/2/22.
//

#include "flare/fiber/future.h"
#include "flare/fiber/fiber.h"
#include "testing/gtest_wrap.h"
#include "flare/fiber/this_fiber.h"
#include "flare/fiber/async.h"
#include "flare/base/fast_rand.h"

using namespace std::literals;

namespace flare {

    TEST(Future, BlockingGet) {
        for (int i = 0; i != 200; ++i) {
            fiber fbs[100];
            std::vector<int> bv{1, 2, 3, 4, 5};
            for (auto &&f : fbs) {
                auto op = [](void *) -> void * {
                    auto v = fiber_future_get(fiber_async(launch_policy::eLazy, [] {
                        std::vector<int> v{1, 2, 3, 4, 5};
                        int round = flare::base::fast_rand_less_than(10);
                        for (int j = 0; j != round; ++j) {
                            flare::fiber_yield();
                        }
                        return v;
                    }));
                    std::vector<int> basw{1, 2, 3, 4, 5};
                    EXPECT_EQ(v, basw);
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
            flare::fiber_sleep_for(1000000);
            f = true;
        });
        auto ts = flare::time_point::from_unix_micros(flare::get_current_time_micros() + 1000000).to_timespec();
        ASSERT_FALSE(fiber_future_try_get_until(std::move(future), &ts));
        ASSERT_FALSE(f);
        flare::fiber_sleep_for(2000000);
        ASSERT_TRUE(f);
    }

}  // namespace flare

//
// Created by liyinbin on 2022/2/22.
//

#include <thread>
#include "flare/fiber/async.h"
#include "testing/gtest_wrap.h"
#include "flare/fiber/future.h"
#include "flare/fiber/this_fiber.h"

using namespace std::literals;

namespace flare {

    TEST(Async, Execute) {
            for (int i = 0; i != 10000; ++i) {
                int rc = 0;
                auto tid = std::this_thread::get_id();
                future<> ff = fiber_async(launch_policy::eImmediately, [&] {
                    rc = 1;
                    //ASSERT_EQ(tid, std::this_thread::get_id())<<"tid: "<<tid<<"std::this_thread::get_id(): "<<std::this_thread::get_id();
                });
                fiber_future_get(&ff);
                ASSERT_EQ(1, rc);
                future<int> f = fiber_async([&] {
                    // Which thread is running this flare is unknown. No assertion here.
                    return 5;
                });
                flare::fiber_yield();
                ASSERT_EQ(5, fiber_future_get(&f));
            }
        }

}  // namespace flare
